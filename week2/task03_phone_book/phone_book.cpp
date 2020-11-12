#include "phone_book.h"

#include <algorithm>
#include <vector>

using namespace std;

PhoneBook::PhoneBook(std::vector<Contact> contacts) {
    sort(contacts.begin(), contacts.end(), [](const Contact &lhs, const Contact &rhs) { return lhs.name < rhs.name; });

    contact_list_protobuf = BuildContacts(contacts);

    contact_list_plain = move(contacts);
}

PhoneBookSerialize::ContactList PhoneBook::BuildContacts(const std::vector<Contact> &sorted_contacts) {
    PhoneBookSerialize::ContactList contact_list;

    for (auto &contact_in : sorted_contacts) {
        PhoneBookSerialize::Contact *cur_contact_ptr = contact_list.add_contact();

        cur_contact_ptr->set_name(contact_in.name);
        if (contact_in.birthday) {
            cur_contact_ptr->mutable_birthday()->set_day(contact_in.birthday->day);
            cur_contact_ptr->mutable_birthday()->set_month(contact_in.birthday->month);
            cur_contact_ptr->mutable_birthday()->set_year(contact_in.birthday->year);
        }
        for (auto &num : contact_in.phones) {
            cur_contact_ptr->add_phone_number(num);
        }
    }

    return contact_list;
}



PhoneBook::ContactRange PhoneBook::FindByNamePrefix(std::string_view name_prefix) const {
    if (name_prefix.empty()) {
        return ContactRange(contact_list_plain.begin(), contact_list_plain.end());
    }

    auto low_it = lower_bound(contact_list_plain.begin(), contact_list_plain.end(), name_prefix, [](const Contact &lhs, string_view rhs) { return lhs.name < rhs; });
    string name_prefix_plus = string(name_prefix);
    name_prefix_plus.back()++;
    auto up_it = lower_bound(contact_list_plain.begin(), contact_list_plain.end(), name_prefix_plus, [](const Contact &lhs, string_view rhs) { return lhs.name < rhs; });

    return PhoneBook::ContactRange(low_it, up_it);
}

void PhoneBook::SaveTo(std::ostream &output) const {
    contact_list_protobuf.SerializeToOstream(&output);
}

vector<Contact> BuildContactsFromProto(PhoneBookSerialize::ContactList contacts_deserialized) {
    vector<Contact> res;
    res.reserve(contacts_deserialized.contact_size());
    for (auto& contact : *contacts_deserialized.mutable_contact()) {
        res.emplace_back();
        res.back().name = move(*contact.mutable_name());
        if (contact.has_birthday()) {
            res.back().birthday = {contact.birthday().year(), contact.birthday().month(), contact.birthday().day()};
        }
        res.back().phones.reserve(contact.phone_number_size());
        for (auto &ph : *contact.mutable_phone_number()) {
            res.back().phones.push_back(move(ph));
        }
    }
    return res;
}

PhoneBook DeserializePhoneBook(std::istream &input) {
    PhoneBookSerialize::ContactList contact_list_in;
    contact_list_in.ParseFromIstream(&input);
    return PhoneBook(BuildContactsFromProto(contact_list_in));
}
