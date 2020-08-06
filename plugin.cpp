/*
 * Fledge OMFHint filter plugin.
 *
 * Copyright (c) 2020 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */

#include <string>
#include <iostream>
#include <plugin_api.h>
#include <config_category.h>
#include <filter.h>
#include <reading_set.h>
#include <version.h>
#include <omfhint.h>

#define HINTS QUOTE({ "asset" : { "number" : "float64" }})

#define FILTER_NAME "omfhint"

const char *default_config = QUOTE({
	"plugin" : {
		"description" : "OMF Hint filter plugin, add OMF hints to one or more assets",
		"type" : "string",
		"default" : FILTER_NAME,
		"readonly" : "true"
		},
	 "enable": {
		"description": "A switch that can be used to enable or disable execution of the filter.",
		"type": "boolean",
		"displayName" : "Enabled",
		"default": "false",
		"order" : "2"
		},
	"hints" : {
		"description" : "OMF hints to add to readings.",
		"type" : "JSON",
		"order" : "1",
		"displayName" : "OMF Hint",
		"default": HINTS
		}
	 });


using namespace std;

/**
 * The Filter plugin interface
 */
extern "C" {

/**
 * The plugin information structure
 */
static PLUGIN_INFORMATION info = {
		FILTER_NAME,              // Name
		VERSION,                  // Version
		0,                        // Flags
		PLUGIN_TYPE_FILTER,       // Type
		"1.0.0",                  // Interface version
		default_config	          // Default plugin configuration
};


/**
 * Return the information about this plugin
 */
PLUGIN_INFORMATION *plugin_info()
{
	return &info;
}

/**
 * Initialise the plugin, called to get the plugin handle and setup the
 * output handle that will be passed to the output stream. The output stream
 * is merely a function pointer that is called with the output handle and
 * the new set of readings generated by the plugin.
 *     (*output)(outHandle, readings);
 * Note that the plugin may not call the output stream if the result of
 * the filtering is that no readings are to be sent onwards in the chain.
 * This allows the plugin to discard data or to buffer it for aggregation
 * with data that follows in subsequent calls
 *
 * @param config	The configuration category for the filter
 * @param outHandle	A handle that will be passed to the output stream
 * @param output	The output stream (function pointer) to which data is passed
 * @return		An opaque handle that is used in all subsequent calls to the plugin
 */
PLUGIN_HANDLE plugin_init(ConfigCategory* config,
			  OUTPUT_HANDLE *outHandle,
			  OUTPUT_STREAM output)
{
	OMFHintFilter *omfhint = new OMFHintFilter(FILTER_NAME, *config, outHandle, output);
	
	return (PLUGIN_HANDLE)omfhint;
}

/**
 * Ingest a set of readings into the plugin for processing
 *
 * @param handle	The plugin handle returned from plugin_init
 * @param readingSet	The readings to process
 */
void plugin_ingest(PLUGIN_HANDLE *handle,
		   READINGSET *readingSet)
{
	OMFHintFilter *omfhint = (OMFHintFilter *)handle;
	if (omfhint)
	{
		if (!omfhint->isEnabled())
		{
			// Current filter is not active: just pass the readings set
			omfhint->m_func(omfhint->m_data, readingSet);
			return;
		}
		vector<Reading *>out;
		omfhint->ingest(readingSet->getAllReadingsPtr(), out);
		delete (ReadingSet *)readingSet;

		ReadingSet *newReadingSet = new ReadingSet(&out);
		const vector<Reading *>& readings = newReadingSet->getAllReadings();
		omfhint->m_func(omfhint->m_data, newReadingSet);
	}
}
/*
 * Plugin reconfigure
 */
void plugin_reconfigure(PLUGIN_HANDLE *handle, const string& newConfig)
{
	OMFHintFilter *omfhint = (OMFHintFilter *)handle;

	if (omfhint)
	{
		omfhint->reconfigure(newConfig);
	}
}

/**
 * Call the shutdown method in the plugin
 */
void plugin_shutdown(PLUGIN_HANDLE *handle)
{
	OMFHintFilter *omfhint = (OMFHintFilter *)handle;

	if (omfhint)
	{
		delete omfhint;
	}
}

// End of extern "C"
};

