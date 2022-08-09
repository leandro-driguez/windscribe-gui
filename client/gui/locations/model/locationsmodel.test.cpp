#include <QtTest>
#include "locationsmodel.test.h"
#include "locationsmodeldata.test.h"
#include "types/locationid.h"
#include "locations/locationsmodel_roles.h"

void TestLocationsModel::init()
{
    testLocations_ = TestData::createBasicTestData();
    customConfigLocation_ = TestData::createBasicCustomConfigTestData();

    locationsModel_.reset(new gui::LocationsModel());
    tester1_.reset(new QAbstractItemModelTester(locationsModel_.get(), QAbstractItemModelTester::FailureReportingMode::QtTest, this));

    citiesModel_.reset(new gui::CitiesModel());
    tester2_.reset(new QAbstractItemModelTester(citiesModel_.get(), QAbstractItemModelTester::FailureReportingMode::QtTest, this));
    citiesModel_->setSourceModel(locationsModel_.get());

    locationsModel_->updateLocations(bestLocation_, testLocations_);
    locationsModel_->updateCustomConfigLocation(customConfigLocation_);
}

void TestLocationsModel::cleanup()
{
    testLocations_.clear();
    bestLocation_ = LocationID();
}

void TestLocationsModel::testBasic()
{
    QVERIFY(locationsModel_->columnCount() == 1);
    QVERIFY(isModelsCorrect(bestLocation_, testLocations_, customConfigLocation_) == true);

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createTopApiLocationId(2));
        QVERIFY(ind.isValid());
        QVERIFY(ind.data(gui::IS_TOP_LEVEL_LOCATION).toBool() == true);
        QVERIFY(ind.data(gui::IS_SHOW_AS_PREMIUM).toBool() == false);
        QVERIFY(ind.data(gui::IS_10GBPS).toBool() == false);
        QVERIFY(ind.data(gui::LOAD).toInt() == 40);
        QVERIFY(ind.data(gui::PING_TIME).toInt() == 203);
    }

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createApiLocationId(1, "City2", "Nick2"));
        QVERIFY(ind.isValid());
        QVERIFY(ind.data(gui::IS_TOP_LEVEL_LOCATION).toBool() == false);
        QVERIFY(ind.data(gui::IS_SHOW_AS_PREMIUM).toBool() == false);
        QVERIFY(ind.data(gui::IS_10GBPS).toBool() == true);
        QVERIFY(ind.data(gui::LOAD).toInt() == 80);
        QVERIFY(ind.data(gui::PING_TIME).toInt() == 55);
    }

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createTopApiLocationId(5));
        QVERIFY(!ind.isValid());
    }

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createStaticIpsLocationId("City2", "Nick2"));
        QVERIFY(ind.isValid());
    }
}

void TestLocationsModel::testBestLocation()
{
    {
        bestLocation_ = LocationID::createApiLocationId(2, "City2", "Nick2").apiLocationToBestLocation();
        locationsModel_->updateBestLocation(bestLocation_);
        QVERIFY(isModelsCorrect(bestLocation_, testLocations_, customConfigLocation_) == true);

        QModelIndex indBestLocation = locationsModel_->getIndexByLocationId(bestLocation_);
        QVERIFY(indBestLocation.isValid());
        QModelIndex indBestLocation2 = locationsModel_->getBestLocationIndex();
        QVERIFY(indBestLocation == indBestLocation2);

        QVERIFY(indBestLocation.data(gui::NAME).toString() == "Best Location");
        QVERIFY(indBestLocation.data(gui::COUNTRY_CODE).toString() == "au");
        QVERIFY(indBestLocation.data(gui::IS_10GBPS).toBool() == true);
        QVERIFY(qvariant_cast<LocationID>(indBestLocation.data(gui::LOCATION_ID)) == bestLocation_);
    }

    {
        bestLocation_ = LocationID::createApiLocationId(1, "City3", "Nick3").apiLocationToBestLocation();
        locationsModel_->updateBestLocation(bestLocation_);
        QVERIFY(isModelsCorrect(bestLocation_, testLocations_, customConfigLocation_) == false);

        QModelIndex indBestLocation = locationsModel_->getIndexByLocationId(bestLocation_);
        QVERIFY(!indBestLocation.isValid());
        QModelIndex indBestLocation2 = locationsModel_->getBestLocationIndex();
        QVERIFY(!indBestLocation2.isValid());
    }
}

void TestLocationsModel::testCustomConfig()
{
    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createCustomConfigLocationId("aaa.ovpn"));
        QVERIFY(ind.isValid());
        QVERIFY(ind.data(gui::CUSTOM_CONFIG_TYPE).toString() == "ovpn");
        QVERIFY(ind.data(gui::IS_CUSTOM_CONFIG_CORRECT).toBool() == true);
    }
    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createCustomConfigLocationId("bbb.conf"));
        QVERIFY(ind.isValid());
        QVERIFY(ind.data(gui::CUSTOM_CONFIG_TYPE).toString() == "wg");
        QVERIFY(ind.data(gui::IS_CUSTOM_CONFIG_CORRECT).toBool() == false);
        QVERIFY(ind.data(gui::CUSTOM_CONFIG_ERROR_MESSAGE).toString() == "error msg");
    }
    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createCustomConfigLocationId("bbb2.conf"));
        QVERIFY(!ind.isValid());
    }
}

void TestLocationsModel::testConnectionSpeed()
{
    bestLocation_ = LocationID::createApiLocationId(1, "City2", "Nick2").apiLocationToBestLocation();
    locationsModel_->updateBestLocation(bestLocation_);

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createTopApiLocationId(1));
        QVERIFY(ind.data(gui::PING_TIME).toInt() == 102);
    }
    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createApiLocationId(1, "City2", "Nick2"));
        QVERIFY(ind.data(gui::PING_TIME).toInt() == 55);
    }
    {
        QModelIndex ind = locationsModel_->getBestLocationIndex();
        QVERIFY(ind.data(gui::PING_TIME).toInt() == 55);
    }

    locationsModel_->changeConnectionSpeed(LocationID::createApiLocationId(1, "City2", "Nick2"), 500);

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createTopApiLocationId(1));
        QVERIFY(ind.data(gui::PING_TIME).toInt() == 325);
    }
    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createApiLocationId(1, "City2", "Nick2"));
        QVERIFY(ind.data(gui::PING_TIME).toInt() == 500);
    }
    {
        QModelIndex ind = locationsModel_->getBestLocationIndex();
        QVERIFY(ind.data(gui::PING_TIME).toInt() == 500);
    }

    {
        QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createCustomConfigLocationId("aaa.ovpn"));
        QVERIFY(ind.data(gui::PING_TIME).toInt() == -2);
        locationsModel_->changeConnectionSpeed(LocationID::createCustomConfigLocationId("aaa.ovpn"), 1500);
        QVERIFY(ind.data(gui::PING_TIME).toInt() == 1500);
    }
}

void TestLocationsModel::testAddCountry()
{
    {
        types::LocationItem l;
        l.name = "Country5";
        l.id = LocationID::createTopApiLocationId(5);
        l.countryCode = "JP";
        l.isPremiumOnly = true;
        l.isP2P = true;
        TestData::addCity(&l, 1, 200, false, false, 10000, 100);
        TestData::addCity(&l, 2, 222, false, false, 10000, 100);
        testLocations_ << l;
    }
    locationsModel_->updateLocations(bestLocation_, testLocations_);
    QVERIFY(isModelsCorrect(bestLocation_, testLocations_, customConfigLocation_) == true);
}

void TestLocationsModel::testAddCity()
{
    TestData::addCity(&testLocations_[2], 1, 200, false, false, 10000, 100);
    TestData::addCity(&testLocations_[2], 2, 200, false, false, 10000, 100);
    TestData::addCity(&testLocations_[3], 2, 333, false, false, 10000, 100);
    locationsModel_->updateLocations(bestLocation_, testLocations_);
    QVERIFY(isModelsCorrect(bestLocation_, testLocations_, customConfigLocation_) == true);
}

void TestLocationsModel::testDeleteCity()
{
    testLocations_ = TestData::createTestDataWithDeletedCities();
    locationsModel_->updateLocations(bestLocation_, testLocations_);
    QVERIFY(isModelsCorrect(bestLocation_, testLocations_, customConfigLocation_) == true);
}

void TestLocationsModel::testDeleteCountry()
{
    testLocations_ = TestData::createTestDataWithDeletedCountries();
    locationsModel_->updateLocations(bestLocation_, testLocations_);
    QVERIFY(isModelsCorrect(bestLocation_, testLocations_, customConfigLocation_) == true);
}

void TestLocationsModel::testChangedCities()
{
    testLocations_ = TestData::createTestDataWithChangedCities();
    locationsModel_->updateLocations(bestLocation_, testLocations_);
    QVERIFY(isModelsCorrect(bestLocation_, testLocations_, customConfigLocation_) == true);
}

void TestLocationsModel::testChangedCities2()
{
    testLocations_ = TestData::createTestDataWithChangedCities2();
    locationsModel_->updateLocations(bestLocation_, testLocations_);
    QVERIFY(isModelsCorrect(bestLocation_, testLocations_, customConfigLocation_) == true);
}

void TestLocationsModel::testFreeSessionStatusChange()
{
    QModelIndex ind = locationsModel_->getIndexByLocationId(LocationID::createApiLocationId(2, "City3", "Nick3"));
    QVERIFY(ind.data(gui::IS_SHOW_AS_PREMIUM).toBool() == false);
    locationsModel_->setFreeSessionStatus(true);
    QVERIFY(ind.data(gui::IS_SHOW_AS_PREMIUM).toBool() == true);
}

bool TestLocationsModel::isModelsCorrect(const LocationID &bestLocation, const QVector<types::LocationItem> &locations, const types::LocationItem &customConfigLocation)
{
    return isLocationsModelEqualTo(bestLocation, locations, customConfigLocation) &&
           isCitiesModelEqualTo(locations, customConfigLocation);
}

bool TestLocationsModel::isLocationsModelEqualTo(const LocationID &bestLocation, const QVector<types::LocationItem> &locations,
                                                 const types::LocationItem &customConfigLocation)
{
    QSet<int> foundIndexes;
    LocationID foundBestLocation;
    int handledCount = 0;

    int g = locationsModel_->rowCount();
    for (int i = 0; i < locationsModel_->rowCount(); ++i)
    {
        QModelIndex miCountry = locationsModel_->index(i, 0);
        LocationID lid = qvariant_cast<LocationID>(miCountry.data(gui::LOCATION_ID));
        if (lid.isBestLocation())
        {
            foundBestLocation = lid;
            handledCount++;
        }
        else if (lid.isCustomConfigsLocation())
        {
            if (!isCountryEqual(miCountry,customConfigLocation))
            {
                return false;
            }
            handledCount++;
        }
        else
        {
            for (int l = 0; l < locations.size(); ++l)
            {
                if (isCountryEqual(miCountry,locations[l]))
                {
                    foundIndexes.insert(l);
                    handledCount++;
                    break;
                }
            }

        }
    }
    if (handledCount != locationsModel_->rowCount()) return false;
    return foundBestLocation == bestLocation && foundIndexes.size() == locations.size();
}

bool TestLocationsModel::isCountryEqual(const QModelIndex &miCountry, const types::LocationItem &l)
{
    // Some fields are skipped for compare
    if (miCountry.data(gui::NAME).toString() !=  l.name) return false;
    if (qvariant_cast<LocationID>(miCountry.data(gui::LOCATION_ID)) != l.id) return false;
    if (miCountry.data(gui::COUNTRY_CODE).toString() !=  l.countryCode.toLower()) return false;
    if (miCountry.data(gui::IS_SHOW_P2P).toBool() !=  l.isP2P) return false;

    int citiesCount = miCountry.model()->rowCount(miCountry);
    if (citiesCount != l.cities.size()) return false;

    QSet<int> foundCitiesIndexes;
    int equalCitiesCount = 0;

    for (int i = 0; i < citiesCount; ++i)
    {
        QModelIndex miCity = miCountry.model()->index(i, 0, miCountry);
        for (int k = 0; k < l.cities.size(); k++)
        {
            if (isCityEqual(miCity, l.cities[k]))
            {
                foundCitiesIndexes.insert(k);
                equalCitiesCount++;
                break;
            }
        }
    }

    return equalCitiesCount == citiesCount && foundCitiesIndexes.size() == citiesCount;
}

bool TestLocationsModel::isCityEqual(const QModelIndex &miCity, const types::CityItem &city)
{
    // Some fields are skipped for compare
    if (miCity.data(gui::NAME).toString() !=  city.city) return false;
    if (city.id.isStaticIpsLocation())
    {
        if (miCity.data(gui::NICKNAME).toString() !=  city.staticIp) return false;
    }
    else
    {
        if (miCity.data(gui::NICKNAME).toString() !=  city.nick) return false;
    }
    if (qvariant_cast<LocationID>(miCity.data(gui::LOCATION_ID)) != city.id) return false;
    if (miCity.data(gui::PING_TIME).toInt() !=  city.pingTimeMs.toInt()) return false;
    if (miCity.data(gui::IS_DISABLED).toBool() !=  city.isDisabled) return false;

    if (miCity.data(gui::STATIC_IP).toString() != city.staticIp) return false;
    if (miCity.data(gui::STATIC_IP_TYPE).toString() !=  city.staticIpType) return false;

    if (miCity.data(gui::IS_CUSTOM_CONFIG_CORRECT).toBool() != city.customConfigIsCorrect) return false;
    if (miCity.data(gui::CUSTOM_CONFIG_ERROR_MESSAGE).toString() !=  city.customConfigErrorMessage) return false;

    if (miCity.data(gui::LINK_SPEED).toInt() !=  city.linkSpeed) return false;

    return true;
}

bool TestLocationsModel::isCitiesModelEqualTo(const QVector<types::LocationItem> &locations, const types::LocationItem &customConfigLocation)
{
    QVector<types::CityItem> cities;
    int handledCount = 0;
    for (int l = 0; l < locations.size(); ++l)
    {
        for (int c = 0; c < locations[l].cities.size(); ++c)
        {
            cities << locations[l].cities[c];
        }
    }

    for (int c = 0; c < customConfigLocation.cities.size(); ++c)
    {
        cities << customConfigLocation.cities[c];
    }

    for (int i = 0; i < citiesModel_->rowCount(); ++i)
    {
        QModelIndex miCity = citiesModel_->index(i, 0);

        for (int c = 0; c < cities.size(); ++c)
        {
            if (isCityEqual(miCity, cities[c]))
            {
                handledCount++;
                break;
            }
        }
    }
    return handledCount == cities.size();
}


QTEST_MAIN(TestLocationsModel)


