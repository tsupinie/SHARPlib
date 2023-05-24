#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <stdexcept>
#include <cmath>

#include "doctest.h"
#include <SHARPlib/layer.h>


TEST_CASE("Testing HeightLayer structs") {
    CHECK_THROWS_AS(
        sharp::HeightLayer layer2( 1000.0, 0.0 ), const std::range_error&
        );
}


TEST_CASE("Testing PressureLayer structs") {
    CHECK_THROWS_AS(
        sharp::PressureLayer layer2( 100.0, 1000.0 ), const std::range_error&
        );
}


TEST_CASE("Testing layer_max over pressure layer") {

    float pres[10] = {1000.0, 900.0, 800.0, 700.0, 600.0, 
                      500.0, 400.0, 300.0, 200.0, 100.0};
    float data[10] = {0, 0, 0, 0, 10, 0, 0, 0, 0, 0};
    sharp::PressureLayer layer1(1000.0, 600.0); // max at top of layer
    sharp::PressureLayer layer2(1100.0, 50.0);  // out of bounds recovery
    sharp::PressureLayer layer3(1000.0, 850.0); // max is not in layer
    sharp::PressureLayer layer4(800.0, 650.0); // max is just above layer
    float pmax = -9999.0;

    CHECK(sharp::layer_max(layer1, pres, data, 10, &pmax) == 10.0);
    CHECK(pmax == 600.0);

    CHECK(sharp::layer_max(layer2, pres, data, 10, nullptr) == 10.0);
    CHECK(sharp::layer_max(layer3, pres, data, 10, nullptr) ==  0.0);

    CHECK(sharp::layer_max(layer4, pres, data, 10, &pmax) ==  
                                     doctest::Approx(4.80749));
    CHECK(pmax == 650.0);
}


TEST_CASE("Testing layer_min over pressure layer") {

    float pres[10] = {1000.0, 900.0, 800.0, 700.0, 600.0, 
                      500.0, 400.0, 300.0, 200.0, 100.0};
    float data[10] = {0, 0, 0, 0, -10, 0, 0, 0, 0, 0};
    sharp::PressureLayer layer1(1000.0, 600.0); // max at top of layer
    sharp::PressureLayer layer2(1100.0, 50.0);  // out of bounds recovery
    sharp::PressureLayer layer3(1000.0, 850.0); // max is not in layer
    sharp::PressureLayer layer4(800.0, 650.0); // max is just above layer

    CHECK(sharp::layer_min(layer1, pres, data, 10) == -10.0);
    CHECK(sharp::layer_min(layer2, pres, data, 10) == -10.0);
    CHECK(sharp::layer_min(layer3, pres, data, 10) ==   0.0);

    CHECK(sharp::layer_min(layer4, pres, data, 10) ==  
                                    doctest::Approx(-4.80749));
}


TEST_CASE("Testing layer_max over height layer") {

    float hght[10] = {1000.0, 2000.0, 3000.0, 4000.0, 5000.0, 
                      6000.0, 7000.0, 8000.0, 9000.0, 10000.0};
    float data[10] = {0, 0, 0, 0, 10, 0, 0, 0, 0, 0};
    sharp::HeightLayer layer1(1000.0, 6000.0); // max at top of layer
    sharp::HeightLayer layer2(-100.0, 20000.0);  // out of bounds recovery
    sharp::HeightLayer layer3(6000.0, 10000.0); // max is not in layer
    sharp::HeightLayer layer4(1000.0, 4750.0); // max is just above layer
    sharp::HeightLayer layer5(5250.0, 9000.0); // max is just below layer

    CHECK(sharp::layer_max(layer1, hght, data, 10, nullptr) == 10.0);
    CHECK(sharp::layer_max(layer2, hght, data, 10, nullptr) == 10.0);
    CHECK(sharp::layer_max(layer3, hght, data, 10, nullptr) ==  0.0);
    CHECK(sharp::layer_max(layer4, hght, data, 10, nullptr) ==  7.5);
    CHECK(sharp::layer_max(layer5, hght, data, 10, nullptr) ==  7.5);
}


TEST_CASE("Testing layer_min over height layer") {

    float hght[10] = {1000.0, 2000.0, 3000.0, 4000.0, 5000.0, 
                      6000.0, 7000.0, 8000.0, 9000.0, 10000.0};
    float data[10] = {0, 0, 0, 0, -10, 0, 0, 0, 0, 0};
    sharp::HeightLayer layer1(1000.0, 6000.0); // max at top of layer
    sharp::HeightLayer layer2(-100.0, 20000.0);  // out of bounds recovery
    sharp::HeightLayer layer3(6000.0, 10000.0); // max is not in layer
    sharp::HeightLayer layer4(1000.0, 4750.0); // max is just above layer
    sharp::HeightLayer layer5(5250.0, 9000.0); // max is just below layer

    CHECK(sharp::layer_min(layer1, hght, data, 10) == -10.0);
    CHECK(sharp::layer_min(layer2, hght, data, 10) == -10.0);
    CHECK(sharp::layer_min(layer3, hght, data, 10) ==  0.0);
    CHECK(sharp::layer_min(layer4, hght, data, 10) ==  -7.5);
    CHECK(sharp::layer_min(layer5, hght, data, 10) ==  -7.5);
}

TEST_CASE("Testing layer_mean over a pressure layer") {

	float pres[10] = { 1000.0, 900.0, 800.0, 700.0, 600.0,
		500.0, 400.0, 300.0, 200.0, 100.0 };

	float data[10] = { 0, 0, 0, 0, 10, 0, 0, 0, 0, 0 };

	sharp::PressureLayer layer1(1000.0, 600.0); // max at top of layer
	sharp::PressureLayer layer2(1100.0, 50.0);  // out of bounds recovery
	sharp::PressureLayer layer3(1000.0, 100.0);  // full layer 
	
	CHECK(sharp::layer_mean(layer1, pres, data, 10) == 1.25);
	CHECK(sharp::layer_mean(layer2, pres, data, 10) == doctest::Approx(1.1111));
	CHECK(sharp::layer_mean(layer3, pres, data, 10) == doctest::Approx(1.1111));

}


