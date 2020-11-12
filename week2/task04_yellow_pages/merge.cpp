#include "yellow_pages.h"

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

namespace YellowPages {

    void MergeAttr(Company &company, const Signals &signals, const Providers &providers, const string &attr_name) {
        const google::protobuf::Message *high_priority_sub_message_for_attr_ptr = nullptr;
        uint32_t current_high_priority = 0;
        for (const auto &signal : signals) {
            if (signal.company().GetReflection()->HasField(signal.company(), signal.company().GetDescriptor()->FindFieldByName(attr_name))) {
                if (providers.at(signal.provider_id()).priority() >= current_high_priority) {
                    current_high_priority = providers.at(signal.provider_id()).priority();
                    high_priority_sub_message_for_attr_ptr = &signal.company().GetReflection()->GetMessage(signal.company(), signal.company().GetDescriptor()->FindFieldByName(attr_name));
                }
            }
        }

        if (high_priority_sub_message_for_attr_ptr != nullptr) {
            company.GetReflection()->MutableMessage(&company, company.GetDescriptor()->FindFieldByName(attr_name))->CopyFrom(
                    *high_priority_sub_message_for_attr_ptr
            );
        }
    }

    void
    MergeAttrRepeated(Company &company, const Signals &signals, const Providers &providers, const string &attr_name) {
        vector<const google::protobuf::Message *> high_priority_sub_messages_for_attr_ptr;
        uint32_t current_high_priority = 0;
        for (const auto &signal : signals) {
            if (signal.company().GetReflection()->FieldSize(signal.company(), signal.company().GetDescriptor()->FindFieldByName(attr_name)) > 0) {
                if (providers.at(signal.provider_id()).priority() > current_high_priority) {
                    current_high_priority = providers.at(signal.provider_id()).priority();
                    high_priority_sub_messages_for_attr_ptr.clear();
                }
                if (providers.at(signal.provider_id()).priority() == current_high_priority) {
                    for (int i = 0; i < signal.company().GetReflection()->FieldSize(signal.company(), signal.company().GetDescriptor()->FindFieldByName(attr_name)); i++) {

                        high_priority_sub_messages_for_attr_ptr.push_back(&signal.company().GetReflection()->GetRepeatedMessage(signal.company(), signal.company().GetDescriptor()->FindFieldByName(attr_name), i));
                    }
                }
            }
        }
        for (const google::protobuf::Message *sub_mes_ptr : high_priority_sub_messages_for_attr_ptr) {
            bool exists = false;
            for (int i = 0; i < company.GetReflection()->FieldSize(company, company.GetDescriptor()->FindFieldByName(attr_name)); i++) {

                if (company.GetReflection()->GetRepeatedMessage(company, company.GetDescriptor()->FindFieldByName(attr_name), i).SerializeAsString() ==
                    sub_mes_ptr->SerializeAsString()) {
                    exists = true;
                };
            }
            if (!exists) {
                company.GetReflection()->AddMessage(&company, company.GetDescriptor()->FindFieldByName(attr_name))->CopyFrom(*sub_mes_ptr);
            }
        }
    }

    Company Merge(const Signals &signals, const Providers &providers) {
        // Наивная неправильная реализация: берём все данные из самого первого сигнала, никак не учитываем приоритеты
        Company company;

        for (const auto &attr_name : {"address", "working_time"}) {
            MergeAttr(company, signals, providers, attr_name);
        }

        for (const auto &attr_name_repeated : {"names", "phones", "urls"}) {
            MergeAttrRepeated(company, signals, providers, attr_name_repeated);
        }

        return company;
    }

}
