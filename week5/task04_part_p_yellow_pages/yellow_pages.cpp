#include "yellow_pages.h"

YellowPages::YellowPages(const Json::Dict &yellow_pages_json) {
    for (const auto&[r_id, rubric_json] : yellow_pages_json.at("rubrics_").AsMap()) {
        rubrics_[rubric_json.AsMap().at("name").AsString()] = std::stoull(r_id);

        if (rubric_json.AsMap().count("keywords")) {
            for (const auto &kw : rubric_json.AsMap().at("keywords").AsArray()) {
                rubrics_[kw.AsString()] = std::stoull(r_id);
            }
        }
    }

    for (const auto& company : yellow_pages_json.at("rubrics_").AsArray()) {

    }
}
