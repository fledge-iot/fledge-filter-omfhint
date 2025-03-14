#include <gtest/gtest.h>
#include <plugin_api.h>
#include <config_category.h>
#include <filter_plugin.h>
#include <filter.h>
#include <string.h>
#include <string>
#include <rapidjson/document.h>
#include <reading.h>
#include <reading_set.h>

using namespace std;
using namespace rapidjson;

extern "C"
{
    PLUGIN_INFORMATION *plugin_info();
    void plugin_ingest(void *handle,
                       READINGSET *readingSet);
    PLUGIN_HANDLE plugin_init(ConfigCategory *config,
                              OUTPUT_HANDLE *outHandle,
                              OUTPUT_STREAM output);
    void plugin_shutdown(PLUGIN_HANDLE handle);
    int called = 0;

    void Handler(void *handle, READINGSET *readings)
    {
        called++;
        *(READINGSET **)handle = readings;
    }
};

TEST(OMFHINT, OmfHintDisabled)
{
    PLUGIN_INFORMATION *info = plugin_info();
    ConfigCategory *config = new ConfigCategory("omfhint", info->config);
    ASSERT_NE(config, (ConfigCategory *)NULL);
    config->setItemsValueFromDefault();
    ASSERT_EQ(config->itemExists("hints"), true);
    ASSERT_EQ(config->itemExists("enable"), true);
    config->setValue("hints", "{ \"test\":{\"number\":\"uint64\", \"datapoint\":{ \"name\":\"Temperature\",\"integer\":\"uint64\"}} }");
    config->setValue("enable", "false");
    ReadingSet *outReadings;
    void *handle = plugin_init(config, &outReadings, Handler);
    vector<Reading *> *readings = new vector<Reading *>;

    long testValue = 2;
    DatapointValue dpv(testValue);
    Datapoint *value = new Datapoint("test", dpv);
    Reading *in = new Reading("test", value);
    readings->push_back(in);

    ReadingSet *readingSet = new ReadingSet(readings);
    readings->clear();
    delete readings;
    plugin_ingest(handle, (READINGSET *)readingSet);

    vector<Reading *> results = outReadings->getAllReadings();
    ASSERT_EQ(results.size(), 1);
    Reading *out = results[0];
    ASSERT_EQ(out->getDatapointCount(), 1);

    vector<Datapoint *> points = out->getReadingData();
    ASSERT_EQ(points.size(), 1);

    delete config;
    delete outReadings;
    plugin_shutdown(handle);
}

TEST(OMFHINT, OmfHintAddAssetDataPointHint)
{
    PLUGIN_INFORMATION *info = plugin_info();
    ConfigCategory *config = new ConfigCategory("omfhint", info->config);
    ASSERT_NE(config, (ConfigCategory *)NULL);
    config->setItemsValueFromDefault();
    ASSERT_EQ(config->itemExists("hints"), true);
    ASSERT_EQ(config->itemExists("enable"), true);
    config->setValue("hints", "{ \"test\" : {\"number\" : \"uint64\", \"datapoint\" : { \"name\" : \"Temperature\",\"integer\" : \"uint64\"}} }");
    config->setValue("enable", "true");
    ReadingSet *outReadings;
    void *handle = plugin_init(config, &outReadings, Handler);
    vector<Reading *> *readings = new vector<Reading *>;

    long testValue = 2;
    DatapointValue dpv(testValue);
    Datapoint *value = new Datapoint("test", dpv);
    Reading *in = new Reading("test", value);
    readings->push_back(in);

    ReadingSet *readingSet = new ReadingSet(readings);
    readings->clear();
    delete readings;
    plugin_ingest(handle, (READINGSET *)readingSet);

    vector<Reading *> results = outReadings->getAllReadings();
    ASSERT_EQ(results.size(), 1);
    Reading *out = results[0];
    ASSERT_EQ(out->getDatapointCount(), 2);

    vector<Datapoint *> points = out->getReadingData();
    ASSERT_EQ(points.size(), 2);

    Datapoint *outdp = points[0];
    ASSERT_STREQ(outdp->getName().c_str(), "test");
    
    outdp = points[1];
    ASSERT_STREQ(outdp->getName().c_str(), "OMFHint");

    delete config;
    delete outReadings;
    plugin_shutdown(handle);
}

TEST(OMFHINT, OmfHintDatapointMacro)
{
    const char *hintsJSON = R"({
     "motor4": {
                "datapoint": [{
                                "name": "voltage",
                                "number": "float32",
                                "uom": "$voltage_uom$"
                        },
                        {
                                "name": "current",
                                "number": "uint32",
                                "uom": "$current_uom$"
                        }
                ]
     }
    })";

    PLUGIN_INFORMATION *info = plugin_info();
    ConfigCategory *config = new ConfigCategory("omfhint", info->config);
    ASSERT_NE(config, (ConfigCategory *)NULL);
    config->setItemsValueFromDefault();
    ASSERT_EQ(config->itemExists("hints"), true);
    ASSERT_EQ(config->itemExists("enable"), true);
    config->setValue("hints", hintsJSON);
    config->setValue("enable", "true");

    ReadingSet *outReadings;
    void *handle = plugin_init(config, &outReadings, Handler);
    vector<Reading *> *readings = new vector<Reading *>;
    vector<Datapoint *> dpValue;

    long voltage = 25;
    DatapointValue voltageDpv(voltage);
    dpValue.push_back(new Datapoint("voltage", voltageDpv));

    string voltage_uom = "Volt";
    DatapointValue voltage_uomDpv(voltage_uom);
    dpValue.push_back( new Datapoint("voltage_uom",voltage_uomDpv) );

    long current = 12;
    DatapointValue currentDpv(current);
    dpValue.push_back(new Datapoint("current", currentDpv));

    string current_uom = "Ampere";
    DatapointValue current_uomDpv(current_uom);
    dpValue.push_back( new Datapoint("current_uom",current_uomDpv) );

    Reading *in = new Reading("motor4", dpValue);
    readings->push_back(in);

    ReadingSet *readingSet = new ReadingSet(readings);
    readings->clear();
    delete readings;
    plugin_ingest(handle, (READINGSET *)readingSet);

    vector<Reading *> results = outReadings->getAllReadings();
    ASSERT_EQ(results.size(), 1);
    Reading *out = results[0];
    ASSERT_EQ(out->getDatapointCount(), 5);

    vector<Datapoint *> points = out->getReadingData();
    ASSERT_EQ(points.size(), 5);

    Datapoint *outdp = points[0];
    ASSERT_STREQ(outdp->getName().c_str(), "voltage");

    outdp = points[1];
    ASSERT_STREQ(outdp->getName().c_str(), "voltage_uom");

    outdp = points[2];
    ASSERT_STREQ(outdp->getName().c_str(), "current");

    outdp = points[3];
    ASSERT_STREQ(outdp->getName().c_str(), "current_uom");

    outdp = points[4];
    ASSERT_STREQ(outdp->getName().c_str(),"OMFHint");
    ASSERT_STREQ(outdp->getData().toString().c_str(), "\"{\\\"datapoint\\\":[{\\\"name\\\":\\\"voltage\\\",\\\"number\\\":\\\"float32\\\",\\\"uom\\\":\\\"Volt\\\"},{\\\"name\\\":\\\"current\\\",\\\"number\\\":\\\"uint32\\\",\\\"uom\\\":\\\"Ampere\\\"}]}\"");

    delete config;
    delete outReadings;
    plugin_shutdown(handle);
}

// Testing for ASSET macro
TEST(OMFHINT, OmfHintASSETMacro)
{
    const char *hintsJSON = R"({"Camera": {"AFLocation" : "/UK/$city$/$factory$/$floor$/$ASSET$" }})";

    PLUGIN_INFORMATION *info = plugin_info();
    ConfigCategory *config = new ConfigCategory("omfhint", info->config);
    ASSERT_NE(config, (ConfigCategory *)NULL);
    config->setItemsValueFromDefault();
    ASSERT_EQ(config->itemExists("hints"), true);
    ASSERT_EQ(config->itemExists("enable"), true);
    config->setValue("hints", hintsJSON);
    config->setValue("enable", "true");

    ReadingSet *outReadings;
    void *handle = plugin_init(config, &outReadings, Handler);
    vector<Reading *> *readings = new vector<Reading *>;
    vector<Datapoint *> dpValue;

    string city = "London";
    DatapointValue cityDpv(city);
    dpValue.push_back(new Datapoint("city", cityDpv));

    string factory = "Plant1";
    DatapointValue factoryDpv(factory);
    dpValue.push_back( new Datapoint("factory",factoryDpv) );

    long floor = 12;
    DatapointValue floorDpv(floor);
    dpValue.push_back(new Datapoint("floor", floorDpv));

    Reading *in = new Reading("Camera", dpValue);
    readings->push_back(in);

    ReadingSet *readingSet = new ReadingSet(readings);
    readings->clear();
    delete readings;
    plugin_ingest(handle, (READINGSET *)readingSet);

    vector<Reading *> results = outReadings->getAllReadings();
    ASSERT_EQ(results.size(), 1);
    Reading *out = results[0];
    ASSERT_EQ(out->getDatapointCount(), 4);

    vector<Datapoint *> points = out->getReadingData();
    ASSERT_EQ(points.size(), 4);

    Datapoint *outdp = points[0];
    ASSERT_STREQ(outdp->getName().c_str(), "city");

    outdp = points[1];
    ASSERT_STREQ(outdp->getName().c_str(), "factory");

    outdp = points[2];
    ASSERT_STREQ(outdp->getName().c_str(), "floor");
    ASSERT_STREQ(outdp->getData().toString().c_str(), "12");

    outdp = points[3];
    ASSERT_STREQ(outdp->getName().c_str(), "OMFHint");
    ASSERT_STREQ(outdp->getData().toString().c_str(),   "\"{\\\"AFLocation\\\":\\\"/UK/London/Plant1/12/Camera\\\"}\"");

    delete config;
    delete outReadings;
    plugin_shutdown(handle);
}

// Testing for missing datapoint city
TEST(OMFHINT, OmfHintASSETMacroMissingDataPoints)
{
    const char *hintsJSON = R"({"Camera": {"AFLocation" : "/UK/$city$/$factory$/$floor$/$ASSET$" }})";

    PLUGIN_INFORMATION *info = plugin_info();
    ConfigCategory *config = new ConfigCategory("omfhint", info->config);
    ASSERT_NE(config, (ConfigCategory *)NULL);
    config->setItemsValueFromDefault();
    ASSERT_EQ(config->itemExists("hints"), true);
    ASSERT_EQ(config->itemExists("enable"), true);
    config->setValue("hints", hintsJSON);
    config->setValue("enable", "true");
    
    ReadingSet *outReadings;
    void *handle = plugin_init(config, &outReadings, Handler);
    vector<Reading *> *readings = new vector<Reading *>;
    vector<Datapoint *> dpValue;

    string factory = "Plant1";
    DatapointValue factoryDpv(factory);
    dpValue.push_back( new Datapoint("factory",factoryDpv) );

    long floor = 12;
    DatapointValue floorDpv(floor);
    dpValue.push_back(new Datapoint("floor", floorDpv));

    Reading *in = new Reading("Camera", dpValue);
    readings->push_back(in);

    ReadingSet *readingSet = new ReadingSet(readings);
    readings->clear();
    delete readings;
    plugin_ingest(handle, (READINGSET *)readingSet);

    vector<Reading *> results = outReadings->getAllReadings();
    ASSERT_EQ(results.size(), 1);
    Reading *out = results[0];
    ASSERT_EQ(out->getDatapointCount(), 3);

    vector<Datapoint *> points = out->getReadingData();
    ASSERT_EQ(points.size(), 3);

    Datapoint *outdp = points[0];
    ASSERT_STREQ(outdp->getName().c_str(), "factory");

    outdp = points[1];
    ASSERT_STREQ(outdp->getName().c_str(), "floor");

    outdp = points[2];
    ASSERT_STREQ(outdp->getName().c_str(), "OMFHint");
    // Except missing datapoint city other macros have been replaced.
    ASSERT_STREQ(outdp->getData().toString().c_str(),  "\"{\\\"AFLocation\\\":\\\"/UK/$city$/Plant1/12/Camera\\\"}\"");

    delete config;
    delete outReadings;
    plugin_shutdown(handle);
}


// Testing for different permutations for macro
TEST(OMFHINT, OmfHintPermutationMacro)
{
    const char *hintsJSON = R"({"Camera": {"AFLocation" : "/UK/North$city$$factory$South/A$floor$B/$ASSET$" }})";

    PLUGIN_INFORMATION *info = plugin_info();
    ConfigCategory *config = new ConfigCategory("omfhint", info->config);
    ASSERT_NE(config, (ConfigCategory *)NULL);
    config->setItemsValueFromDefault();
    ASSERT_EQ(config->itemExists("hints"), true);
    ASSERT_EQ(config->itemExists("enable"), true);
    config->setValue("hints", hintsJSON);
    config->setValue("enable", "true");

    ReadingSet *outReadings;
    void *handle = plugin_init(config, &outReadings, Handler);
    vector<Reading *> *readings = new vector<Reading *>;
    vector<Datapoint *> dpValue;

    string city = "London";
    DatapointValue cityDpv(city);
    dpValue.push_back(new Datapoint("city", cityDpv));

    string factory = "Plant1";
    DatapointValue factoryDpv(factory);
    dpValue.push_back( new Datapoint("factory",factoryDpv) );

    long floor = 12;
    DatapointValue floorDpv(floor);
    dpValue.push_back(new Datapoint("floor", floorDpv));

    Reading *in = new Reading("Camera", dpValue);
    readings->push_back(in);

    ReadingSet *readingSet = new ReadingSet(readings);
    readings->clear();
    delete readings;
    plugin_ingest(handle, (READINGSET *)readingSet);

    vector<Reading *> results = outReadings->getAllReadings();
    ASSERT_EQ(results.size(), 1);
    Reading *out = results[0];
    ASSERT_EQ(out->getDatapointCount(), 4);

    vector<Datapoint *> points = out->getReadingData();
    ASSERT_EQ(points.size(), 4);

    Datapoint *outdp = points[0];
    ASSERT_STREQ(outdp->getName().c_str(), "city");

    outdp = points[1];
    ASSERT_STREQ(outdp->getName().c_str(), "factory");

    outdp = points[2];
    ASSERT_STREQ(outdp->getName().c_str(), "floor");
    ASSERT_STREQ(outdp->getData().toString().c_str(), "12");

    outdp = points[3];
    ASSERT_STREQ(outdp->getName().c_str(), "OMFHint");
    ASSERT_STREQ(outdp->getData().toString().c_str(),  "\"{\\\"AFLocation\\\":\\\"/UK/NorthLondonPlant1South/A12B/Camera\\\"}\"");

    delete config;
    delete outReadings;
    plugin_shutdown(handle);
}

// Testing for unpaired $ sign
TEST(OMFHINT, OmfHintPermutationMacro2)
{
    const char *hintsJSON = R"({"Camera": {"AFLocation" : "/UK/$city$$factory$/$floor$ASSET$" }})";

    PLUGIN_INFORMATION *info = plugin_info();
    ConfigCategory *config = new ConfigCategory("omfhint", info->config);
    ASSERT_NE(config, (ConfigCategory *)NULL);
    config->setItemsValueFromDefault();
    ASSERT_EQ(config->itemExists("hints"), true);
    ASSERT_EQ(config->itemExists("enable"), true);
    config->setValue("hints", hintsJSON);
    config->setValue("enable", "true");

    ReadingSet *outReadings;
    void *handle = plugin_init(config, &outReadings, Handler);
    vector<Reading *> *readings = new vector<Reading *>;
    vector<Datapoint *> dpValue;

    string city = "London";
    DatapointValue cityDpv(city);
    dpValue.push_back(new Datapoint("city", cityDpv));

    string factory = "Plant1";
    DatapointValue factoryDpv(factory);
    dpValue.push_back( new Datapoint("factory",factoryDpv) );

    long floor = 12;
    DatapointValue floorDpv(floor);
    dpValue.push_back(new Datapoint("floor", floorDpv));

    Reading *in = new Reading("Camera", dpValue);
    readings->push_back(in);

    ReadingSet *readingSet = new ReadingSet(readings);
    readings->clear();
    delete readings;
    plugin_ingest(handle, (READINGSET *)readingSet);

    vector<Reading *> results = outReadings->getAllReadings();
    ASSERT_EQ(results.size(), 1);
    Reading *out = results[0];
    ASSERT_EQ(out->getDatapointCount(), 4);

    vector<Datapoint *> points = out->getReadingData();
    ASSERT_EQ(points.size(), 4);

    Datapoint *outdp = points[0];
    ASSERT_STREQ(outdp->getName().c_str(), "city");

    outdp = points[1];
    ASSERT_STREQ(outdp->getName().c_str(), "factory");

    outdp = points[2];
    ASSERT_STREQ(outdp->getName().c_str(), "floor");
    ASSERT_STREQ(outdp->getData().toString().c_str(), "12");

    outdp = points[3];
    ASSERT_STREQ(outdp->getName().c_str(), "OMFHint");
    ASSERT_STREQ(outdp->getData().toString().c_str(),  "\"{\\\"AFLocation\\\":\\\"/UK/LondonPlant1/12ASSET$\\\"}\"");

    delete config;
    delete outReadings;
    plugin_shutdown(handle);
}

// Testing for ASSET macro
TEST(OMFHINT, OmfHintCaseSensitivityMacro)
{
    const char *hintsJSON = R"({"Camera": {"AFLocation" : "/UK/$city$/$Factory$/$Floor$/$ASSET$" }})";

    PLUGIN_INFORMATION *info = plugin_info();
    ConfigCategory *config = new ConfigCategory("omfhint", info->config);
    ASSERT_NE(config, (ConfigCategory *)NULL);
    config->setItemsValueFromDefault();
    ASSERT_EQ(config->itemExists("hints"), true);
    ASSERT_EQ(config->itemExists("enable"), true);
    config->setValue("hints", hintsJSON);
    config->setValue("enable", "true");

    ReadingSet *outReadings;
    void *handle = plugin_init(config, &outReadings, Handler);
    vector<Reading *> *readings = new vector<Reading *>;
    vector<Datapoint *> dpValue;

    string city = "London";
    DatapointValue cityDpv(city);
    dpValue.push_back(new Datapoint("city", cityDpv));

    string factory = "Plant1";
    DatapointValue factoryDpv(factory);
    dpValue.push_back( new Datapoint("factory",factoryDpv) );

    long floor = 12;
    DatapointValue floorDpv(floor);
    dpValue.push_back(new Datapoint("floor", floorDpv));

    Reading *in = new Reading("Camera", dpValue);
    readings->push_back(in);

    ReadingSet *readingSet = new ReadingSet(readings);
    readings->clear();
    delete readings;
    plugin_ingest(handle, (READINGSET *)readingSet);

    vector<Reading *> results = outReadings->getAllReadings();
    ASSERT_EQ(results.size(), 1);
    Reading *out = results[0];
    ASSERT_EQ(out->getDatapointCount(), 4);

    vector<Datapoint *> points = out->getReadingData();
    ASSERT_EQ(points.size(), 4);

    Datapoint *outdp = points[0];
    ASSERT_STREQ(outdp->getName().c_str(), "city");

    outdp = points[1];
    ASSERT_STREQ(outdp->getName().c_str(), "factory");

    outdp = points[2];
    ASSERT_STREQ(outdp->getName().c_str(), "floor");
    ASSERT_STREQ(outdp->getData().toString().c_str(), "12");

    outdp = points[3];
    ASSERT_STREQ(outdp->getName().c_str(), "OMFHint");
    ASSERT_STREQ(outdp->getData().toString().c_str(),  "\"{\\\"AFLocation\\\":\\\"/UK/London/$Factory$/$Floor$/Camera\\\"}\"");

    delete config;
    delete outReadings;
    plugin_shutdown(handle);
}

// Testing macro for unsupported datatype for city datapoint
TEST(OMFHINT, OmfHintUnspportedMacro)
{
    const char *hintsJSON = R"({"Camera": {"AFLocation" : "/UK/$city$/$factory$/$floor$/$ASSET$" }})";

    PLUGIN_INFORMATION *info = plugin_info();
    ConfigCategory *config = new ConfigCategory("omfhint", info->config);
    ASSERT_NE(config, (ConfigCategory *)NULL);
    config->setItemsValueFromDefault();
    ASSERT_EQ(config->itemExists("hints"), true);
    ASSERT_EQ(config->itemExists("enable"), true);
    config->setValue("hints", hintsJSON);
    config->setValue("enable", "true");

    ReadingSet *outReadings;
    void *handle = plugin_init(config, &outReadings, Handler);
    vector<Reading *> *readings = new vector<Reading *>;
    vector<Datapoint *> dpValue;

    //unsupported DatapointValue::dataTagType::T_IMAGE datatype
    void *data = malloc(100 * 100);
    DPImage *cityImage = new DPImage(100, 100, 8, data);
    free(data);
    
    DatapointValue cityDpv(cityImage);
    dpValue.push_back(new Datapoint("city", cityDpv));

    string factory = "Plant1";
    DatapointValue factoryDpv(factory);
    dpValue.push_back( new Datapoint("factory",factoryDpv) );

    long floor = 12;
    DatapointValue floorDpv(floor);
    dpValue.push_back(new Datapoint("floor", floorDpv));

    Reading *in = new Reading("Camera", dpValue);
    readings->push_back(in);

    ReadingSet *readingSet = new ReadingSet(readings);
    readings->clear();
    delete readings;
    plugin_ingest(handle, (READINGSET *)readingSet);

    vector<Reading *> results = outReadings->getAllReadings();
    ASSERT_EQ(results.size(), 1);
    Reading *out = results[0];
    ASSERT_EQ(out->getDatapointCount(), 4);

    vector<Datapoint *> points = out->getReadingData();
    ASSERT_EQ(points.size(), 4);

    Datapoint *outdp = points[0];
    ASSERT_STREQ(outdp->getName().c_str(), "city");

    outdp = points[1];
    ASSERT_STREQ(outdp->getName().c_str(), "factory");

    outdp = points[2];
    ASSERT_STREQ(outdp->getName().c_str(), "floor");
    ASSERT_STREQ(outdp->getData().toString().c_str(), "12");

    outdp = points[3];
    ASSERT_STREQ(outdp->getName().c_str(), "OMFHint");
    // Except unsupported datapoint city of image type other macros have been replaced.
    ASSERT_STREQ(outdp->getData().toString().c_str(),  "\"{\\\"AFLocation\\\":\\\"/UK/$city$/Plant1/12/Camera\\\"}\"");

    delete config;
    delete outReadings;
    plugin_shutdown(handle);
}
