#include "yellow_pages_search.h"

namespace YellowPagesSearch {
    CompanyNameConstraint::CompanyNameConstraint(const std::vector<Json::Node> &names_json) {
        for (const auto &name_json : names_json) {
            names_.insert(name_json.AsString());
        }
    }

    bool CompanyNameConstraint::is_suite(const ::YellowPagesDatabase::Company &company_to_check) const {
        if (names_.empty()) { return true; }

        return std::any_of(company_to_check.get_company_names().begin(),
                           company_to_check.get_company_names().end(),
                           [this](const YellowPagesDatabase::CompanyName &company_name) {
                               return static_cast<bool>(names_.count(company_name.get_name()));
                           });

    }

    CompanyPhoneConstraint::OnePhoneConstraint::OnePhoneConstraint(const Json::Node &one_phone_json_node) {
        const Json::Dict &one_phone_json = one_phone_json_node.AsMap();
        if (one_phone_json.count("type")) {
            if (one_phone_json.at("type").AsString() == "PHONE") {
                type_ = ::YellowPages::Phone_Type::Phone_Type_PHONE;
            } else if (one_phone_json.at("type").AsString() == "FAX") {
                type_ = ::YellowPages::Phone_Type::Phone_Type_FAX;
            }
        }
        if (one_phone_json.count("country_code")) {
            country_code_ = one_phone_json.at("country_code").AsString();
        }
        if (one_phone_json.count("local_code")) {
            local_code_ = one_phone_json.at("local_code").AsString();
        }
        if (one_phone_json.count("number")) {
            number_ = one_phone_json.at("number").AsString();
        }
        if (one_phone_json.count("extension") && !one_phone_json.at("extension").AsString().empty()) {  // пустой == отсутствует
            extension_ = one_phone_json.at("extension").AsString();
        }
    }

    bool CompanyPhoneConstraint::OnePhoneConstraint::is_one_phone_suite(const YellowPagesDatabase::CompanyPhone &phone_to_check) const {
        if ((this->extension_.has_value() && !phone_to_check.extension_.has_value())
            || (this->extension_.has_value() && phone_to_check.extension_.has_value() && *this->extension_ != *phone_to_check.extension_)) {
            return false;
        }
//        if (this->type_.has_value() && !phone_to_check.type_.has_value() ||
//            this->type_.has_value() && phone_to_check.type_.has_value() && *this->type_ != phone_to_check.type_) {
        if (this->type_.has_value() && *this->type_ != phone_to_check.type_) {
            return false;
        }
        if ((this->country_code_.has_value() && !phone_to_check.country_code_.has_value()) ||
            (this->country_code_.has_value() && phone_to_check.country_code_.has_value() && *this->country_code_ != *phone_to_check.country_code_)) {
            return false;
        }
        if ((this->country_code_.has_value() || this->local_code_.has_value()) &&
            ((this->local_code_.has_value() && !phone_to_check.local_code_.has_value()) ||
             (this->local_code_.has_value() && phone_to_check.local_code_.has_value() && *this->local_code_ != *phone_to_check.local_code_))
                ) {
            return false;
        }

        if ((this->number_.has_value() && !phone_to_check.number_.has_value()) ||
            (!this->number_.has_value() && phone_to_check.number_.has_value()) ||
            (this->number_.has_value() && phone_to_check.number_.has_value() && *this->number_ != *phone_to_check.number_)) {
            return false;
        } else if ((this->number_.has_value() && phone_to_check.number_.has_value() && *this->number_ == *phone_to_check.number_) ||
                   (!this->number_.has_value() && !phone_to_check.number_.has_value())) {
            return true;
        }
        throw std::runtime_error("CompanyPhoneConstraint::OnePhoneConstraint::is_one_phone_suite(const YellowPagesDatabase::CompanyPhone &phone_to_check)");
    }

    CompanyPhoneConstraint::CompanyPhoneConstraint(const std::vector<Json::Node> &names_json) : phone_constraints_(names_json.begin(), names_json.end()) {}

    bool CompanyPhoneConstraint::is_suite(const YellowPagesDatabase::Company &company_to_check) const {
        if (phone_constraints_.empty()) { return true; }

        return std::any_of(company_to_check.get_company_phones().begin(),
                           company_to_check.get_company_phones().end(),
                           [this](const YellowPagesDatabase::CompanyPhone &company_phone) {
                               return std::any_of(phone_constraints_.begin(),
                                                  phone_constraints_.end(),
                                                  [&company_phone](const OnePhoneConstraint &one_phone_constraint) {
                                                      return one_phone_constraint.is_one_phone_suite(company_phone);
                                                  }
                               );
                           });
    }

    CompanyUrlConstraint::CompanyUrlConstraint(const std::vector<Json::Node> &urls_json) {
        for (const auto &url_json : urls_json) {
            urls_.insert(url_json.AsString());
        }
    }

    bool CompanyUrlConstraint::is_suite(const YellowPagesDatabase::Company &company_to_check) const {
        if (urls_.empty()) { return true; }

        return std::any_of(company_to_check.get_company_urls().begin(),
                           company_to_check.get_company_urls().end(),
                           [this](const std::string &company_url) {
                               return urls_.count(company_url);
                           });
    }

    CompanyRubricConstraint::CompanyRubricConstraint(const std::vector<Json::Node> &rubric_strs_json,
                                                     const std::unordered_map<std::string, uint64_t> &rubric_ids_dict) {
        for (const auto &one_rubric_str_json : rubric_strs_json) {
            rubrics_.insert(
                    rubric_ids_dict.at(one_rubric_str_json.AsString())
            );
        }
    }

    bool CompanyRubricConstraint::is_suite(const YellowPagesDatabase::Company &company_to_check) const {
        if (rubrics_.empty()) { return true; }

        return std::any_of(company_to_check.get_company_rubrics().begin(),
                           company_to_check.get_company_rubrics().end(),
                           [this](uint64_t company_rubric) {
                               return rubrics_.count(company_rubric);
                           });
    }
}