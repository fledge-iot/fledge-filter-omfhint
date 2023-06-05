/*
 * Fledge OMFHint filter plugin.
 *
 * Copyright (c) 2020 Dianomic Systems
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mark Riddoch
 */
#include <filter.h>
#include <reading_set.h>
#include <config_category.h>
#include <string>
#include <map>
#include <regex>

class OMFHintFilter : public FledgeFilter {
	public:
		OMFHintFilter(const std::string& filterName,
			ConfigCategory& filterConfig,
			OUTPUT_HANDLE *outHandle,
			OUTPUT_STREAM out);
		void	ingest(std::vector<Reading *> *in, std::vector<Reading *>& out);
		void	reconfigure(const std::string& newConfig);
	private:
		void	configure(const ConfigCategory& config);
		void	collectMacrosInfo(std::string hintsJSON);
		void	ReplaceMacros(Reading * reading, std::string& hintsJSON);

		std::map<std::string, std::string>               m_hints;
		std::vector<std::pair<std::string, int>>         m_macro_dp;
		std::vector<std::pair<std::regex, std::string>> m_wildcards;
};
