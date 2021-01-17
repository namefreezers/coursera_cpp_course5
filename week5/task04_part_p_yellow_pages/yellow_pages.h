#pragma once

#include <array>

#include "company.pb.h"
#include "database.pb.h"
#include "name.pb.h"
#include "phone.pb.h"

#include "json.h"
#include "yellow_pages_search.h"

namespace YellowPagesSearch {
    class CompanyConstraint;

    class CompanyNameConstraint;

    class CompanyPhoneConstraint;

    class CompanyRubricConstraint;

    class CompanyUrlConstraint;
}


namespace YellowPagesDatabase {

    struct Coords {
        double latitude_;
        double longitude_;
    };

    class CompanyAddress {
    public:
        explicit CompanyAddress(const Json::Dict &company_address_json);

//        explicit CompanyAddress(const YellowPages::Address &serialization_company_address);

    private:
        std::optional<std::string> formatted_;
//        std::vector<CompanyAddressComponent> components_;
        std::optional<Coords> coords_;
        std::optional<std::string> comment_;

    };

    class CompanyName {
    public:
//        enum class CompanyNameType {
//            MAIN = 0,
//            SYNONYM = 1,
//            SHORT = 2,
//        };

        explicit CompanyName(const Json::Dict &company_name_json);

        explicit CompanyName(const YellowPages::Name &serialization_company_name);

        const std::string &get_name() const;

        bool is_main_name() const;

        YellowPages::Name SerializeName() const;


    private:
        std::string value_;
        YellowPages::Name_Type type_;
    };

    class CompanyPhone {
//        friend bool YellowPagesSearch::CompanyPhoneConstraint::OnePhoneConstraint::is_one_phone_suite(const CompanyPhone &phone_to_check) const;  // todo

    public:
        explicit CompanyPhone(const Json::Dict &company_phone_json);

        explicit CompanyPhone(const YellowPages::Phone &serialization_company_phone);

        YellowPages::Phone SerializePhone() const;

    public:  // todo:
        std::optional<std::string> formatted_;
        YellowPages::Phone_Type type_;
        std::optional<std::string> country_code_;
        std::optional<std::string> local_code_;
        std::optional<std::string> number_;
        std::optional<std::string> extension_;
        std::optional<std::string> description_;
    };

    class Company {
    public:
        explicit Company(const Json::Dict &company_json);

        explicit Company(const YellowPages::Company &serialization_company);

        const std::string& get_main_name() const;

        const std::vector<CompanyName> &get_company_names() const;

        const std::vector<CompanyPhone> &get_company_phones() const;

        const std::vector<std::string> &get_company_urls() const;

        const std::vector<uint64_t> &get_company_rubrics() const;

        ::YellowPages::Company SerializeCompany() const;

    private:
//        std::optional<CompanyAddress> address_;
        std::vector<CompanyName> names_;
        std::optional<std::string> main_name_;
        std::vector<CompanyPhone> phones_;
        std::vector<std::string> urls_;  // todo: std::unordered_set
        std::vector<uint64_t> rubrics_;  // todo: std::unordered_set
        // CompanyWorkingTime working_time_;
//        std::unordered_map<std::string, uint32_t> nearby_stops_;
    };

    class YellowPagesDb {
    public:
        YellowPagesDb() = default;

        explicit YellowPagesDb(const Json::Dict &yellow_pages_json);

        explicit YellowPagesDb(const ::YellowPages::Database &serialization_yellow_pages);

        const std::unordered_map<std::string, uint64_t> &get_rubric_ids_dict() const;

        std::vector<const YellowPagesDatabase::Company *> SearchCompanies(const ::std::array<const YellowPagesSearch::CompanyConstraint *, 4> &companies_constraint_ptrs) const;

        ::YellowPages::Database SerializeYellowPages() const;

    private:
        std::unordered_map<std::string, uint64_t> rubric_ids_;
        std::unordered_map<uint64_t, std::string> main_rubric_names_;
        std::unordered_map<uint64_t, std::vector<std::string>> keyword_rubric_names_;

        std::vector<Company> companies_;
    };


}