#pragma once

#include "contact.pb.h"
#include "iterator_range.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <iosfwd>

struct Date {
    int year, month, day;
};

struct Contact {
    std::string name;
    std::optional<Date> birthday;
    std::vector<std::string> phones;
};

class PhoneBook {
public:
    explicit PhoneBook(std::vector<Contact> contacts);

    using ContactRange = IteratorRange<std::vector<Contact>::const_iterator>;

    ContactRange FindByNamePrefix(std::string_view name_prefix) const;

    void SaveTo(std::ostream &output) const;

private:
    static PhoneBookSerialize::ContactList BuildContacts(const std::vector<Contact>& sorted_contacts);


    PhoneBookSerialize::ContactList contact_list_protobuf;

    std::vector<Contact> contact_list_plain;
};

std::vector<Contact> BuildContactsFromProto(PhoneBookSerialize::ContactList contacts_deserialized);

PhoneBook DeserializePhoneBook(std::istream &input);

