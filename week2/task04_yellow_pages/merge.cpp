#include "yellow_pages.h"

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

namespace YellowPages {

    void MergeAttr(Company &company, const Signals &signals, const Providers &providers, const string &attr_name) {
        const google::protobuf::Message *high_priority_message_for_attr_ptr;
        uint32_t current_high_priority = 0;
        for (const auto &signal : signals) {
            if (Company::GetReflection()->HasField(signal.company(), Company::GetDescriptor()->FindFieldByName(attr_name))) {
                if (providers.at(signal.provider_id()).priority() >= current_high_priority) {
                    current_high_priority = providers.at(signal.provider_id()).priority();
                    high_priority_message_for_attr_ptr = &Company::GetReflection()->GetMessage(signal.company(), Company::GetDescriptor()->FindFieldByName(attr_name));
                }
            }
        }
        const google::protobuf::Message &sub_message = Company::GetReflection()->GetMessage(*high_priority_message_for_attr_ptr, Company::GetDescriptor()->FindFieldByName(attr_name));
        google::protobuf::Message *sub_message_copy = Company::GetReflection()->GetMessage(*high_priority_message_for_attr_ptr, Company::GetDescriptor()->FindFieldByName(attr_name)).New();
        sub_message_copy->ParseFromString(sub_message.SerializeAsString());

        Company::GetReflection()->SetAllocatedMessage(&company,
                                                      sub_message_copy,
                                                      Company::GetDescriptor()->FindFieldByName(attr_name));
    }

    void MergeAttrRepeated(Company &company, const Signals &signals, const Providers &providers, const string &attr_name) {
        vector<const google::protobuf::Message *> high_priority_messages_for_attr_ptr;
        uint32_t current_high_priority = 0;
        for (const auto &signal : signals) {
            if (Company::GetReflection()->FieldSize(signal.company(), Company::GetDescriptor()->FindFieldByName(attr_name)) > 0) {
                if (providers.at(signal.provider_id()).priority() > current_high_priority) {
                    current_high_priority = providers.at(signal.provider_id()).priority();
                    high_priority_messages_for_attr_ptr.clear();
                }
                if (providers.at(signal.provider_id()).priority() == current_high_priority) {
                    high_priority_messages_for_attr_ptr.push_back(&Company::GetReflection()->GetMessage(signal.company(), Company::GetDescriptor()->FindFieldByName(attr_name)));
                }
            }
        }
        for (const google::protobuf::Message *mes_ptr : high_priority_messages_for_attr_ptr) {
            for (size_t i = 0; i < Company::GetReflection()->FieldSize(*mes_ptr, Company::GetDescriptor()->FindFieldByName(attr_name)); i++) {
                const google::protobuf::Message &sub_message = Company::GetReflection()->GetRepeatedMessage(*mes_ptr, Company::GetDescriptor()->FindFieldByName(attr_name), i);
                google::protobuf::Message *sub_message_copy = Company::GetReflection()->GetRepeatedMessage(*mes_ptr, Company::GetDescriptor()->FindFieldByName(attr_name), i).New();
                sub_message_copy->ParseFromString(sub_message.SerializeAsString());

                Company::GetReflection()->AddAllocatedMessage(&company,
                                                              Company::GetDescriptor()->FindFieldByName(attr_name),
                                                              sub_message_copy);
            }
        }
    }

    Company Merge(const Signals &signals, const Providers &providers) {
        // Наивная неправильная реализация: берём все данные из самого первого сигнала, никак не учитываем приоритеты
        Company company;

        for (const auto& attr_name : {"address", "working_time"}) {
            MergeAttr(company, signals, providers, attr_name);
        }

        for (const auto& attr_name_repeated : {"names", "phones", "urls"}) {
            MergeAttr(company, signals, providers, attr_name_repeated);
        }

        return company;
    }

}
