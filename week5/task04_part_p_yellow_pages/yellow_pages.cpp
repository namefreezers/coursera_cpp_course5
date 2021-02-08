#include "yellow_pages.h"


namespace YellowPagesDatabase {
    CompanyAddress::CompanyAddress(const Json::Dict &company_address_json) {
//        if (company_address_json.count("formatted")) {
        formatted_ = company_address_json.at("formatted").AsString();
//        }
//        if (company_address_json.count("coords")) {
        coords_ = {company_address_json.at("coords").AsMap().at("lat").AsDouble(), company_address_json.at("coords").AsMap().at("lat").AsDouble()};
//        }
//        if (company_address_json.count("comment")) {
        comment_ = company_address_json.at("comment").AsString();
//        }
    }

//    CompanyAddress::CompanyAddress(const YellowPages::Address &serialization_company_address) {
//    }

    CompanyName::CompanyName(const Json::Dict &company_name_json) {
        value_ = company_name_json.at("value").AsString();
        if (!company_name_json.count("type") || company_name_json.at("type").AsString().empty() || company_name_json.at("type").AsString() == "MAIN") {
            type_ = ::YellowPages::Name_Type::Name_Type_MAIN;
        } else if (company_name_json.at("type").AsString() == "SHORT") {
            type_ = ::YellowPages::Name_Type::Name_Type_SHORT;
        } else if (company_name_json.at("type").AsString() == "SYNONYM") {
            type_ = ::YellowPages::Name_Type::Name_Type_SYNONYM;
        }
    }

    CompanyName::CompanyName(const ::YellowPages::Name &serialization_company_name) {
        value_ = serialization_company_name.value();
        type_ = serialization_company_name.type();
    }

    const std::string &CompanyName::get_name() const {
        return value_;
    }

    bool CompanyName::is_main_name() const {
        return type_ == YellowPages::Name_Type::Name_Type_MAIN;
    }

    ::YellowPages::Name CompanyName::SerializeName() const {
        ::YellowPages::Name serialization_name;
        serialization_name.set_value(value_);
        serialization_name.set_type(type_);
        return serialization_name;
    }

    CompanyPhone::CompanyPhone(const Json::Dict &company_phone_json) {
        if (company_phone_json.count("formatted") && !company_phone_json.at("formatted").AsString().empty()) {
            formatted_ = company_phone_json.at("formatted").AsString();
        }
        if (!company_phone_json.count("type") || company_phone_json.at("type").AsString() == "PHONE") {
            type_ = ::YellowPages::Phone_Type::Phone_Type_PHONE;
        } else if (company_phone_json.count("type") && company_phone_json.at("type").AsString() == "FAX") {
            type_ = ::YellowPages::Phone_Type::Phone_Type_FAX;
        } else {
            throw std::runtime_error("CompanyPhone::CompanyPhone(const Json::Dict &company_phone_json)");
        }

        if (company_phone_json.count("country_code") && !company_phone_json.at("country_code").AsString().empty()) {
            country_code_ = company_phone_json.at("country_code").AsString();
        }
        if (company_phone_json.count("local_code") && !company_phone_json.at("local_code").AsString().empty()) {
            local_code_ = company_phone_json.at("local_code").AsString();
        }
        if (company_phone_json.count("number") && !company_phone_json.at("number").AsString().empty()) {
            number_ = company_phone_json.at("number").AsString();
        }
        if (company_phone_json.count("extension") && !company_phone_json.at("extension").AsString().empty()) {
            extension_ = company_phone_json.at("extension").AsString();
        }
        if (company_phone_json.count("description") && !company_phone_json.at("description").AsString().empty()) {
            description_ = company_phone_json.at("description").AsString();
        }
    }

    CompanyPhone::CompanyPhone(const ::YellowPages::Phone &serialization_company_phone) {
        formatted_ = serialization_company_phone.formatted().empty() ? std::nullopt : std::optional(serialization_company_phone.formatted());
        type_ = serialization_company_phone.type();
        country_code_ = serialization_company_phone.country_code().empty() ? std::nullopt : std::optional(serialization_company_phone.country_code());
        local_code_ = serialization_company_phone.local_code().empty() ? std::nullopt : std::optional(serialization_company_phone.local_code());
        number_ = serialization_company_phone.number().empty() ? std::nullopt : std::optional(serialization_company_phone.number());
        extension_ = serialization_company_phone.extension().empty() ? std::nullopt : std::optional(serialization_company_phone.extension());
        description_ = serialization_company_phone.description().empty() ? std::nullopt : std::optional(serialization_company_phone.description());
    }

    ::YellowPages::Phone CompanyPhone::SerializePhone() const {
        ::YellowPages::Phone serialization_phone;
        serialization_phone.set_formatted(formatted_.has_value() ? *formatted_ : "");
        serialization_phone.set_type(type_);
        serialization_phone.set_country_code(country_code_.has_value() ? *country_code_ : "");
        serialization_phone.set_local_code(local_code_.has_value() ? *local_code_ : "");
        serialization_phone.set_number(number_.has_value() ? *number_ : "");
        serialization_phone.set_extension(extension_.has_value() ? *extension_ : "");
        serialization_phone.set_description(description_.has_value() ? *description_ : "");
        return serialization_phone;
    }


    Company::Company(const Json::Dict &company_json) {
//        if (company_json.count("address")) {
//            address_ = CompanyAddress(company_json.at("address").AsMap());
//        }

        names_.reserve(company_json.at("names").AsArray().size());
        for (const auto &company_name_json : company_json.at("names").AsArray()) {
            names_.emplace_back(company_name_json.AsMap());
            if (!main_name_.has_value() && names_.back().is_main_name()) {
                main_name_ = names_.back().get_name();
            }
        }

        if (company_json.count("phones")) {
            phones_.reserve(company_json.at("phones").AsArray().size());
            for (const auto &company_phone_json : company_json.at("phones").AsArray()) {
                phones_.emplace_back(company_phone_json.AsMap());
            }
        }

        if (company_json.count("urls")) {
            urls_.reserve(company_json.at("urls").AsArray().size());
            for (const auto &company_url_json : company_json.at("urls").AsArray()) {
                urls_.push_back(company_url_json.AsMap().at("value").AsString());
            }
        }

        rubrics_.reserve(company_json.at("rubrics").AsArray().size());
        for (const auto &company_rubric_json : company_json.at("rubrics").AsArray()) {
            rubrics_.push_back(company_rubric_json.AsInt());
        }

//        if (company_json.count("nearby_stops")) {
//            nearby_stops_.reserve(company_json.at("nearby_stops").AsArray().size());
//            for (const auto &company_nearby_stop_json : company_json.at("nearby_stops").AsArray()) {
//                nearby_stops_[company_nearby_stop_json.AsMap().at("name").AsString()] = company_nearby_stop_json.AsMap().at("meters").AsInt();
//            }
//        }
    }

    Company::Company(const ::YellowPages::Company &serialization_company) {
        names_.reserve(serialization_company.names_size());
        for (int name_idx = 0; name_idx < serialization_company.names_size(); ++name_idx) {
            names_.emplace_back(serialization_company.names(name_idx));
            if (!main_name_.has_value() && names_.back().is_main_name()) {
                main_name_ = names_.back().get_name();
            }
        }

        phones_.reserve(serialization_company.phones_size());
        for (int phone_idx = 0; phone_idx < serialization_company.phones_size(); ++phone_idx) {
            phones_.emplace_back(serialization_company.phones(phone_idx));
        }

        urls_.reserve(serialization_company.urls_size());
        for (int url_idx = 0; url_idx < serialization_company.urls_size(); ++url_idx) {
            urls_.push_back(serialization_company.urls(url_idx).value());
        }

        for (int rubric_idx = 0; rubric_idx < serialization_company.rubrics_size(); ++rubric_idx) {
            rubrics_.push_back(serialization_company.rubrics(rubric_idx));
        }
    }

    const std::string &Company::get_main_name() const {
        return *main_name_;
    }

    const std::vector<CompanyName> &Company::get_company_names() const {
        return names_;
    }

    const std::vector<CompanyPhone> &Company::get_company_phones() const {
        return phones_;
    }

    const std::vector<std::string> &Company::get_company_urls() const {
        return urls_;
    }

    const std::vector<uint64_t> &Company::get_company_rubrics() const {
        return rubrics_;
    }

    ::YellowPages::Company Company::SerializeCompany() const {
        ::YellowPages::Company serialization_company;
        for (const auto &n : names_) {
            *serialization_company.add_names() = n.SerializeName();
        }

        for (const auto &ph : phones_) {
            *serialization_company.add_phones() = ph.SerializePhone();
        }

        for (const auto &url : urls_) {
            ::YellowPages::Url &serialization_url = *serialization_company.add_urls();
            serialization_url.set_value(url);
        }

        for (auto rubric_id : rubrics_) {
            serialization_company.add_rubrics(rubric_id);
        }

        return serialization_company;
    }

    YellowPagesDb::YellowPagesDb(const Json::Dict &yellow_pages_json) {

        for (const auto&[r_id_string, rubric_json] : yellow_pages_json.at("rubrics").AsMap()) {
            uint64_t cur_rubric_id = std::stoull(r_id_string);

            main_rubric_names_[cur_rubric_id] = rubric_json.AsMap().at("name").AsString();
            rubric_ids_[rubric_json.AsMap().at("name").AsString()] = cur_rubric_id;

            if (rubric_json.AsMap().count("keywords")) {
                std::vector<std::string> &cur_rubric_keywords = keyword_rubric_names_[cur_rubric_id];
                cur_rubric_keywords.reserve(rubric_json.AsMap().at("keywords").AsArray().size());

                for (const auto &kw : rubric_json.AsMap().at("keywords").AsArray()) {
                    cur_rubric_keywords.push_back(kw.AsString());
                    rubric_ids_[kw.AsString()] = cur_rubric_id;
                }
            }
        }

        companies_.reserve(yellow_pages_json.at("companies").AsArray().size());
        for (const auto &company : yellow_pages_json.at("companies").AsArray()) {
            companies_.emplace_back(company.AsMap());
        }
    }

    YellowPagesDb::YellowPagesDb(const ::YellowPages::Database &serialization_yellow_pages) {

        for (const auto &pair_rubric_id_serialization_rubric: serialization_yellow_pages.rubrics()) {
            uint64_t rubric_id = pair_rubric_id_serialization_rubric.first;
            const ::YellowPages::Rubric &serialization_rubric = pair_rubric_id_serialization_rubric.second;

            rubric_ids_[serialization_rubric.name()] = rubric_id;
            main_rubric_names_[rubric_id] = serialization_rubric.name();

            keyword_rubric_names_[rubric_id].reserve(serialization_rubric.keywords_size());
            for (int rubric_kw_idx = 0; rubric_kw_idx < serialization_rubric.keywords_size(); ++rubric_kw_idx) {
                rubric_ids_[serialization_rubric.keywords(rubric_kw_idx)] = rubric_id;
                keyword_rubric_names_[rubric_id].push_back(serialization_rubric.keywords(rubric_kw_idx));
            }
        }

        companies_.reserve(serialization_yellow_pages.companies_size());
        for (int company_idx = 0; company_idx < serialization_yellow_pages.companies_size(); ++company_idx) {
            companies_.emplace_back(serialization_yellow_pages.companies(company_idx));
        }
    }

    const std::unordered_map<std::string, uint64_t> &YellowPagesDb::get_rubric_ids_dict() const {
        return rubric_ids_;
    }

    std::vector<const YellowPagesDatabase::Company *> YellowPagesDb::SearchCompanies(const std::array<const YellowPagesSearch::CompanyConstraint *, 4> &companies_constraint_ptrs) const {
        std::vector<const YellowPagesDatabase::Company *> res;
        for (const auto &company : companies_) {
            bool is_company_suite = std::all_of(companies_constraint_ptrs.begin(), companies_constraint_ptrs.end(),
                                                [&company](const YellowPagesSearch::CompanyConstraint *cur_consraint_ptr) {
                                                    return cur_consraint_ptr->is_suite(company);
                                                });
            if (is_company_suite) {
                res.push_back(&company);
            }
        }
        return res;
    }

    ::YellowPages::Database YellowPagesDb::SerializeYellowPages() const {
        ::YellowPages::Database serialization_yellow_pages;

        for (const auto &company : companies_) {
            *serialization_yellow_pages.add_companies() = company.SerializeCompany();
        }

        for (const auto&[rubric_id, main_rubric_name] : main_rubric_names_) {
            ::YellowPages::Rubric &serialization_rubric = (*serialization_yellow_pages.mutable_rubrics())[rubric_id];
            serialization_rubric.set_name(main_rubric_name);
            if (keyword_rubric_names_.count(rubric_id)) {
                for (const auto &rubric_keyword : keyword_rubric_names_.at(rubric_id)) {
                    serialization_rubric.add_keywords(rubric_keyword);
                }
            }
        }

        return serialization_yellow_pages;
    }
}