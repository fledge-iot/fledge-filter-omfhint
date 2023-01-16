/*
 * Fledge omfhint filter plugin.
 *
 * Copyright (c) 2020 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */
#include <stdio.h>
#include <reading.h>
#include <reading_set.h>
#include <utility>
#include <logger.h>
#include <rapidjson/document.h>
#include "rapidjson/stringbuffer.h"
#include <rapidjson/writer.h>
#include <omfhint.h>
#include <string_utils.h>
#include <string.h>

using namespace std;
using namespace rapidjson;

/**
 * Constructor for the OMFHint Filter class
 *
 */
OMFHintFilter::OMFHintFilter(const std::string& filterName,
		     ConfigCategory& filterConfig,
		     OUTPUT_HANDLE *outHandle,
		     OUTPUT_STREAM out) :
				FledgeFilter(filterName, filterConfig,
						outHandle, out)
{
	configure(filterConfig);
}



/**
 * Ingest data into the plugin and write the processed data to the out vector
 *
 * @param readings	The readings to process
 * @param out		The output readings vector
 */
void
OMFHintFilter::ingest(vector<Reading *> *readings, vector<Reading *>& out)
{
	AssetTracker *instance =  nullptr;
	instance =  AssetTracker::getAssetTracker();
	// Iterate thru' the readings
 	for (vector<Reading *>::const_iterator elem = readings->begin();
			elem != readings->end(); ++elem)
	{
		string name = (*elem)->getAssetName();
		auto it = m_hints.find(name);

		if (it != m_hints.end())
		{
			DatapointValue value(it->second);
			(*elem)->addDatapoint(new Datapoint("OMFHint", value));
			if (instance != nullptr)
			{
				instance->addAssetTrackingTuple(m_name, name, string("Filter"));
			}
		} else {

			if ( ! m_wildcards.empty() ) {

				for(auto &item : m_wildcards) {

					if (std::regex_match (name, item.first))
					{
						DatapointValue value(item.second);
						(*elem)->addDatapoint(new Datapoint("OMFHint", value));
						if (instance != nullptr)
						{
							instance->addAssetTrackingTuple(m_name, name, string("Filter"));
						}
						break;
					}
				}
			}
		}
		out.push_back(*elem);
	}
	readings->clear();
}


/**
 * Reconfigure the RMS filter
 *
 * @param newConfig	The new configuration
 */
void
OMFHintFilter::reconfigure(const string& newConfig)
{
	setConfig(newConfig);
	ConfigCategory config("config", newConfig);
	configure(config);
}

void
OMFHintFilter::configure(const ConfigCategory& config)
{
	if (config.itemExists("hints"))
	{
		m_hints.clear();
		m_wildcards.clear();

		Document doc;
		ParseResult result = doc.Parse(config.getValue("hints").c_str());
		if (!result)
		{
			Logger::getLogger()->error("Error parsing OMF Hints: %s at %u",
				doc.GetParseError(), result.Offset());
		}
		else
		{
			for (Value::ConstMemberIterator itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr)
			{
				string asset = itr->name.GetString();
				StringBuffer buffer;
				Writer<StringBuffer> writer(buffer);
				itr->value.Accept(writer);
 
				const string hint = buffer.GetString();
				string escaped = hint;
				string replace = "\\\"";
				size_t pos = escaped.find("\"");
				while( pos != std::string::npos)
				{
					escaped.replace(pos, 1, replace);
					pos = escaped.find("\"", pos+replace.size());
				}

				if (IsRegex(asset)) {

					m_wildcards.push_back(std::pair<std::regex, std::string>(std::regex(asset), escaped));
				} else {
					m_hints.insert(pair<string, string>(asset, escaped));
				}
			}
		}
	}
}
