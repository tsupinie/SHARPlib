
/**
 * \file
 * \brief Routines used for parcel lifting and integration
 * \author
 *   Kelton Halbert                  \n
 *   Email: kelton.halbert@noaa.gov  \n
 *   License: Apache 2.0             \n
 * \date   2022-11-09
 *
 * Written for the NWS Storm Predidiction Center \n
 * Based on NSHARP routines originally written by
 * John Hart and Rich Thompson at SPC.
 */
#include <SHARPlib/constants.h>
#include <SHARPlib/interp.h>
#include <SHARPlib/layer.h>
#include <SHARPlib/parcel.h>
#include <SHARPlib/profile.h>
#include <SHARPlib/thermo.h>

namespace sharp {

Parcel::Parcel() {
    // Set LPL values to MISSING
    pres = MISSING;
    tmpc = MISSING;
    dwpc = MISSING;

    // Set derived values to MISSING
    lcl_pressure = MISSING;
    lfc_pressure = MISSING;
    eql_pressure = MISSING;

    cape = 0.0;
    cinh = 0.0;
}

/**
 * \author Kelton Halbert - NWS Storm Prediction Center/OU-CIWRO
 *
 * \brief Sets the parcel initial values to the surface of the profile
 *
 * \param prof
 * \param pcl
 *
 */
void _sfc(Profile* prof, Parcel* pcl) noexcept {
    pcl->pres = prof->pres[0];
    pcl->tmpc = prof->tmpc[0];
    pcl->dwpc = prof->dwpc[0];
}

/**
 * \author Kelton Halbert - NWS Storm Prediction Center/OU-CIWRO
 *
 * \brief Sets the parcel initial values to the most unstable parcel level
 *
 * \param prof
 * \param pcl
 *
 */
void _mu(Profile* prof, Parcel* pcl) noexcept {
    // Search for the most unstable parcel in the bottom
    // 400 hPa of the profile
    PressureLayer mu_layer(prof->pres[0], prof->pres[0] - 400.0);

    // layer_max returns the max, and will set the pressure
    // of the max via a pointer to a float.
    layer_max(mu_layer, prof->pres, prof->theta_e, prof->NZ, &(pcl->pres));
    pcl->tmpc = interp_pressure(pcl->pres, prof->pres, prof->tmpc, prof->NZ);
    pcl->dwpc = interp_pressure(pcl->pres, prof->pres, prof->dwpc, prof->NZ);
}

/**
 * \author Kelton Halbert - NWS Storm Prediction Center/OU-CIWRO
 *
 * \brief Sets the parcel initial values to the bottom 100mb mixed layer
 *
 * \param prof
 * \param pcl
 *
 */
void _ml(Profile* prof, Parcel* pcl) noexcept {
    PressureLayer mix_layer(prof->pres[0], prof->pres[0] - 100.0);

    // get the mean attributes of the lowest 100 hPa
    float mean_mixr = layer_mean(mix_layer, prof->pres, prof->mixr, prof->NZ);
    float mean_thta = layer_mean(mix_layer, prof->pres, prof->theta, prof->NZ);

    // set the parcel attributes
    pcl->pres = prof->pres[0];
    pcl->tmpc = theta(1000.0, mean_thta, prof->pres[0]);
    pcl->dwpc = temperature_at_mixratio(mean_mixr, prof->pres[0]);
}

void define_parcel(Profile* prof, Parcel* pcl, LPL source) noexcept {
    pcl->source = source;

    if (source == LPL::SFC) {
        _sfc(prof, pcl);
        return;
    } else if (source == LPL::FCST) {
        // TO-DO: Write the forecast_surface routine
        return;
    } else if (source == LPL::MU) {
        _mu(prof, pcl);
        return;
    } else if (source == LPL::ML) {
        _ml(prof, pcl);
        return;
    } else if (source == LPL::EIL) {
        // TO-DO: Write the EIL routine
        return;
    } else if (source == LPL::USR) {
        // do nothing - its already been set!
        return;
    } else {
        // TO-DO: probably should raise an error or something
        return;
    }
}

inline bool __is_lfc(float lyr_1, float lyr_2) {
    return ((lyr_1 <= 0) && (lyr_2 > 0));
}

inline bool __is_el(float lyr_1, float lyr_2) {
    return ((lyr_1 >= 0) && (lyr_2 < 0)) ; 
}

void find_lfc_el(Parcel* pcl, float* pres_arr, float* hght_arr, float* buoy_arr,
                 int NZ) noexcept {
    PressureLayer sat_lyr = {pcl->lcl_pressure, pres_arr[NZ - 1]};
    LayerIndex lyr_idx = get_layer_index(sat_lyr, pres_arr, NZ);

    float lyr_bot = 0.0;
    float pos_buoy = 0.0;
    float pos_buoy_last = 0.0;
    float pbot = sat_lyr.bottom;
    float buoy_bot = interp_pressure(sat_lyr.bottom, pres_arr, buoy_arr, NZ);
    float hbot = interp_pressure(sat_lyr.bottom, pres_arr, hght_arr, NZ);

    float lfc_pres = (buoy_bot > 0) ? sat_lyr.bottom : MISSING;
    float eql_pres = MISSING;
    float lfc_pres_last = MISSING;
    float eql_pres_last = MISSING;

    for (int k = lyr_idx.kbot; k <= lyr_idx.ktop + 1; ++k) {
        float ptop = pres_arr[k];
        float htop = hght_arr[k];
        float buoy_top = buoy_arr[k];
        float lyr_top = (buoy_top + buoy_bot) / 2.0f;
        if ((lyr_bot <= 0) && (lyr_top > 0)) {
            if (lfc_pres != MISSING) {
                pos_buoy_last = pos_buoy;
                lfc_pres_last = lfc_pres;
                eql_pres_last = eql_pres;
                pos_buoy = 0.0;
            }
            for (lfc_pres = pbot - 5; lfc_pres >= ptop + 5; lfc_pres -= 1.0) {
                float buoy = interp_pressure(lfc_pres, pres_arr, buoy_arr, NZ);
                if (buoy > 0) break;
            }
        }

        if ((lfc_pres != MISSING) && (lyr_top > 0.0)) {
            pos_buoy += (htop - hbot) * lyr_top;
        }

        if ((lfc_pres != MISSING) && ((lyr_bot >= 0) && (lyr_top < 0))) {
            for (eql_pres = pbot - 5; eql_pres >= ptop + 5; eql_pres -= 1.0) {
                float buoy = interp_pressure(eql_pres, pres_arr, buoy_arr, NZ);
                if (buoy < 0) break;
            }
            if (pos_buoy_last > pos_buoy) {
                lfc_pres = lfc_pres_last;
                eql_pres = eql_pres_last;
                pos_buoy = pos_buoy_last;
            }
        }
        if ((k == NZ - 1) && (lyr_top > 0)) eql_pres = pres_arr[NZ - 1];
        pbot = ptop;
        hbot = htop;
        buoy_bot = buoy_top;
        lyr_bot = lyr_top;
    }
    pcl->lfc_pressure = lfc_pres;
    pcl->eql_pressure = eql_pres;
}

void cape_cinh(Profile* prof, Parcel* pcl) noexcept {
    find_lfc_el(pcl, prof->pres, prof->hght, prof->buoyancy, prof->NZ);
    if ((pcl->lfc_pressure != MISSING) && (pcl->eql_pressure != MISSING)) {
		PressureLayer lfc_el = {pcl->lfc_pressure, pcl->eql_pressure};
		PressureLayer lpl_lfc = {pcl->pres, pcl->lfc_pressure};
        HeightLayer lfc_el_ht =
            pressure_layer_to_height(lfc_el, prof->pres, prof->hght, prof->NZ);
        HeightLayer lpl_lfc_ht =
            pressure_layer_to_height(lpl_lfc, prof->pres, prof->hght, prof->NZ);

        float CINH = integrate_layer_trapz(lpl_lfc_ht, prof->buoyancy,
                                           prof->hght, prof->NZ, -1);
        float CAPE = integrate_layer_trapz(lfc_el_ht, prof->buoyancy,
                                           prof->hght, prof->NZ, 1);
		pcl->cape = CAPE;
		pcl->cinh = CINH;
    }
}

void parcel_wobf(Profile* prof, Parcel* pcl) noexcept {
    constexpr lifter_wobus lifter;
    lift_parcel(lifter, prof, pcl);

    find_lfc_el(pcl, prof->pres, prof->hght, prof->buoyancy, prof->NZ);

    if ((pcl->lfc_pressure != MISSING) && (pcl->eql_pressure != MISSING)) {
		PressureLayer lfc_el = {pcl->lfc_pressure, pcl->eql_pressure};
		PressureLayer lpl_lfc = {pcl->pres, pcl->lfc_pressure};
        HeightLayer lfc_el_ht =
            pressure_layer_to_height(lfc_el, prof->pres, prof->hght, prof->NZ);
        HeightLayer lpl_lfc_ht =
            pressure_layer_to_height(lpl_lfc, prof->pres, prof->hght, prof->NZ);
        float CINH = integrate_layer_trapz(lpl_lfc_ht, prof->buoyancy,
                                           prof->hght, prof->NZ, -1);
        float CAPE = integrate_layer_trapz(lfc_el_ht, prof->buoyancy,
                                           prof->hght, prof->NZ, 1);
		pcl->cape = CAPE;
		pcl->cinh = CINH;
    }
    return;
}

// NCAPE/buoyancy dilution from Peters et al. 2022
float buoyancy_dilution(Profile* prof, Parcel* pcl, const float* mse_star,
                        const float* mse_bar) noexcept {
    // this routine requires there to be a defined
    // LFC and EL, since these are the integration bounds
    if ((pcl->lfc_pressure == MISSING) || (pcl->eql_pressure == MISSING)) {
        return MISSING;
    }

    // set our integration layer between the LFC and EL,
    // and get the top and bottom indices of these
    // layers for iteration
    PressureLayer integ_layer = {pcl->lfc_pressure, pcl->eql_pressure};
    LayerIndex integ_idx = get_layer_index(integ_layer, prof->pres, prof->NZ);

    // start with the interpolated bottom layer
    float mse_bar_bot =
        interp_pressure(integ_layer.bottom, prof->pres, mse_bar, prof->NZ);
    float mse_star_bot =
        interp_pressure(integ_layer.bottom, prof->pres, mse_star, prof->NZ);
    float tmpk_bot =
        interp_pressure(integ_layer.bottom, prof->pres, prof->tmpc, prof->NZ) +
        ZEROCNK;
    float zbot =
        interp_pressure(integ_layer.bottom, prof->pres, prof->hght, prof->NZ);

    // initialize the top layer variables
    // to missing - these get set in the loop
    float mse_bar_top = MISSING;
    float mse_star_top = MISSING;
    float tmpk_top = MISSING;
    float ztop = MISSING;
    float NCAPE = 0.0;

    // kbot is always the first level above the pressure
    // value of the bottom layer, and ktop is always
    // the level just below the top layer
    for (int k = integ_idx.kbot; k <= integ_idx.ktop; k++) {
#ifndef NO_QC
        if ((prof->tmpc[k] == MISSING) || (prof->dwpc[k] == MISSING)) {
            continue;
        }
#endif
        // get the top-layer values
        mse_bar_top = mse_bar[k];
        mse_star_top = mse_star[k];
        tmpk_top = prof->tmpc[k] + ZEROCNK;
        ztop = prof->hght[k];

        // compute the integrated quantity
        float mse_diff_bot = -1.0 * (GRAVITY / (CP_DRYAIR * tmpk_bot)) *
                             (mse_bar_bot - mse_star_bot);
        float mse_diff_top = -1.0 * (GRAVITY / (CP_DRYAIR * tmpk_top)) *
                             (mse_bar_top - mse_star_top);
        float dz = ztop - zbot;

        NCAPE += ((mse_diff_bot + mse_diff_top) / 2.0) * dz;

        // set the top of the current layer
        // as the bottom of the next layer
        mse_bar_bot = mse_bar_top;
        mse_star_bot = mse_star_top;
        tmpk_bot = tmpk_top;
        zbot = ztop;
    }

    // finish with the interpolated top layer
    mse_bar_top =
        interp_pressure(integ_layer.top, prof->pres, mse_bar, prof->NZ);
    mse_star_top =
        interp_pressure(integ_layer.top, prof->pres, mse_star, prof->NZ);
    ztop = interp_pressure(integ_layer.top, prof->pres, prof->hght, prof->NZ);
    tmpk_top =
        interp_pressure(integ_layer.top, prof->pres, prof->tmpc, prof->NZ) +
        ZEROCNK;

    // compute the integrated quantity
    float mse_diff_bot = -1.0 * (GRAVITY / (CP_DRYAIR * tmpk_bot)) *
                         (mse_bar_bot - mse_star_bot);
    float mse_diff_top = -1.0 * (GRAVITY / (CP_DRYAIR * tmpk_top)) *
                         (mse_bar_top - mse_star_top);
    float dz = ztop - zbot;

    NCAPE += ((mse_diff_bot + mse_diff_top) / 2.0) * dz;

    return NCAPE;
}

}  // end namespace sharp

namespace sharp::exper {}  // end namespace sharp::exper
