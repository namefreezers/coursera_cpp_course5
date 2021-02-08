#pragma once

#include <vector>
#include <unordered_set>

#include "yellow_pages.h"


namespace YellowPagesDatabase {
    class Company;
    class CompanyPhone;
    class CompanyName;
}

namespace YellowPagesSearch {
    class CompanyConstraint {
    public:
        virtual bool is_suite(const YellowPagesDatabase::Company &company_to_check) const = 0;
    };

    class CompanyNameConstraint : public CompanyConstraint {
    public:
        explicit CompanyNameConstraint(const std::vector<Json::Node> &names_json);

        bool is_suite(const ::YellowPagesDatabase::Company &company_to_check) const override;

    private:
        std::unordered_set<std::string> names_;
    };

    class CompanyPhoneConstraint : public CompanyConstraint {
    public:
        class OnePhoneConstraint {
        public:
            explicit OnePhoneConstraint(const Json::Node &one_phone_json);

            bool is_one_phone_suite(const YellowPagesDatabase::CompanyPhone &phone_to_check) const;

        private:
            std::optional<::YellowPages::Phone_Type> type_;
            std::optional<std::string> country_code_;
            std::optional<std::string> local_code_;
            std::optional<std::string> number_;
            std::optional<std::string> extension_;
        };

    public:
        explicit CompanyPhoneConstraint(const std::vector<Json::Node> &phones_json);

        bool is_suite(const ::YellowPagesDatabase::Company &company_to_check) const override;

    private:
        std::vector<OnePhoneConstraint> phone_constraints_;
    };

    class CompanyUrlConstraint : public CompanyConstraint {
    public:
        explicit CompanyUrlConstraint(const std::vector<Json::Node> &urls_json);

        bool is_suite(const ::YellowPagesDatabase::Company &company_to_check) const override;

    private:
        std::unordered_set<std::string> urls_;
    };

    class CompanyRubricConstraint : public CompanyConstraint {
    public:
        explicit CompanyRubricConstraint(const std::vector<Json::Node> &names_json, const std::unordered_map<std::string, uint64_t> &rubric_ids_dict);

        bool is_suite(const ::YellowPagesDatabase::Company &company_to_check) const override;

    private:
        std::unordered_set<uint64_t> rubrics_;
    };
}