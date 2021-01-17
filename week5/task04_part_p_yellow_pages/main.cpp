#include <iostream>
#include <fstream>

#include "transport_catalog.pb.h"

#include "json.h"
#include "descriptions.h"
#include "requests.h"
#include "transport_catalog.h"

using namespace std;

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: transport_catalog_part_o [make_base|process_requests]\n";
        return 5;
    }

    const string_view mode(argv[1]);

    if (mode == "make_base") {
        const auto input_doc = Json::Load(cin);
        const auto &input_map = input_doc.GetRoot().AsMap();

        const TransportCatalog db(
                Descriptions::ReadDescriptions(input_map.at("base_requests").AsArray()),
                input_map.at("routing_settings").AsMap(),
                input_map.at("render_settings").AsMap(),
                input_map.at("yellow_pages").AsMap()
        );

        // Save DB
        Serialization::TransportCatalog serialization_base = db.SerializeBase();
        ofstream ofstream_file(input_map.at("serialization_settings").AsMap().at("file").AsString(), ios::binary);
        serialization_base.SerializeToOstream(&ofstream_file);

    } else if (mode == "process_requests") {
        const auto input_doc = Json::Load(cin);
        const auto &input_map = input_doc.GetRoot().AsMap();

        ifstream ifstream_file(input_map.at("serialization_settings").AsMap().at("file").AsString(), ios::binary);
        Serialization::TransportCatalog serialization_base;
        serialization_base.ParseFromIstream(&ifstream_file);

        const TransportCatalog db(serialization_base);

        Json::PrintValue(
                Requests::ProcessAll(db, input_map.at("stat_requests").AsArray()),
                cout
        );
        cout << endl;

    }

    return 0;
}
