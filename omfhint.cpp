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
			std::string hintsJSON = it->second;
			if (!m_macro_dp.empty())
				ReplaceMacros(*elem, hintsJSON);
			DatapointValue value(hintsJSON);
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
						std::string hintsJSON = item.second;
						if (!m_macro_dp.empty())
							ReplaceMacros(*elem, hintsJSON);
						DatapointValue value(hintsJSON);
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

				if (IsRegex(asset))
				{
					try {
						m_wildcards.push_back(std::pair<std::regex, std::string>(std::regex(asset), escaped));
					} catch (const std::regex_error& e) {
						Logger::getLogger()->warn("Asset name %s in OMF hint is not a valid regular expression, it will be treated as a literal asset name.", asset.c_str());
						m_hints.insert(pair<string, string>(asset, escaped));
					}
				}
				else
				{
					m_hints.insert(pair<string, string>(asset, escaped));
				}
				// Check if macro substitution is required
				// At least one pair of '$' sign must be there to apply macro
				if (std::count(escaped.begin(), escaped.end(), '$') > 1)
					collectMacrosInfo(escaped.c_str());
			}
		}
	
	}
}

/**
 * Extract datapoint name for macro replacement
 *
 * @param hintsJson	OMFHints JSON
 */
void OMFHintFilter::collectMacrosInfo(std::string hintsJSON)
{
	std::string::size_type start = hintsJSON.find('$');
	std::string::size_type end = hintsJSON.find('$', start + 1);

	while (start != std::string::npos && end != std::string::npos) 
	{
		if (end > start + 1) 
		{
			m_macro_dp.emplace_back(std::make_pair( hintsJSON.substr(start + 1, end - start - 1), start));
		}
		start = hintsJSON.find('$', end + 1);
		end = hintsJSON.find('$', start + 1);
	}
}

/**
 * Replace Macros with datapoint values
 *
 * @param reading	Reading
 * @param hintsJson	OMFHints JSON
 */
void OMFHintFilter::ReplaceMacros(Reading *reading, std::string &hintsJSON)
{
	// Replace Macros by datapoint value
	for (auto it =  m_macro_dp.rbegin(); it != m_macro_dp.rend(); ++it)
	{
		// In case of ASSET Macro, replace it by asset name instead of datapoint value
		if ((*it).first == "ASSET")
		{
			hintsJSON.replace((*it).second, (*it).first.length()+2, reading->getAssetName() );
			continue;
		}
		Datapoint * datapoint = reading->getDatapoint((*it).first);

		if (datapoint)
		{
			// Check for datapoint type for string and numbers
			DatapointValue::dataTagType dataType = datapoint->getData().getType();
			if (
				dataType != DatapointValue::dataTagType::T_STRING &&
				dataType != DatapointValue::dataTagType::T_INTEGER &&
				dataType != DatapointValue::dataTagType::T_FLOAT
			)
			{
				Logger::getLogger()->warn("The datapoint %s cannot be used as a macro substitution in the OMF Hint as it is not a string or numeric value",(*it).first.c_str());
				continue;
			}
			string datapointValue = "";
			switch (dataType)
			{
				case DatapointValue::dataTagType::T_INTEGER: 
					datapointValue = std::to_string(datapoint->getData().toInt());
					break;
				case DatapointValue::dataTagType::T_FLOAT: 
					datapointValue = std::to_string(datapoint->getData().toDouble());
					break;
				default: 
				datapointValue = datapoint->getData().toStringValue();
				break;
			}
			hintsJSON.replace((*it).second, (*it).first.length()+2, datapointValue );
		}
	}
}
