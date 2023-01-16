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
    plugin_ingest(handle, (READINGSET *)readingSet);

    vector<Reading *> results = outReadings->getAllReadings();
    ASSERT_EQ(results.size(), 1);
    Reading *out = results[0];
    ASSERT_EQ(out->getDatapointCount(), 1);

    vector<Datapoint *> points = out->getReadingData();
    ASSERT_EQ(points.size(), 1);
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
}
