#include "MessageDatabaseManager.hpp"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/uri.hpp>
#include <spdlog/spdlog.h>

#include "utils.hpp"
#include "constants.hpp"

MessageDatabaseManager::MessageDatabaseManager(const std::string& auth_url)
    : connection(mongocxx::uri{auth_url}), db(connection["wheatley"]) {
    load_channel_thread_stati();
}

MessageDatabaseReader MessageDatabaseManager::make_message_database_reader() {
    auto excluded_channels = private_channel_list();
    for(const auto& channel : blacklisted_channels) {
        excluded_channels.append(channel);
    }
    auto filter = bsoncxx::builder::basic::make_document(
        bsoncxx::builder::basic::kvp(
            "channel",
            bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("$nin", excluded_channels))
        ),
        bsoncxx::builder::basic::kvp(
            "deleted",
            bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("$exists", false))
        )
    );
    mongocxx::options::find opts;
    opts.sort(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("timestamp", 1)));
    // std::cout<<bsoncxx::to_json(filter)<<std::endl;
    auto cursor = db["message_database"].find(std::move(filter), std::move(opts));
    return MessageDatabaseReader(std::move(cursor), private_channels);
}

bsoncxx::builder::basic::array MessageDatabaseManager::private_channel_list() const {
    bsoncxx::builder::basic::array arr;
    for(const auto& id : private_channels) {
        arr.append(id);
    }
    return arr;
}

void MessageDatabaseManager::load_channel_thread_stati() {
    auto cursor = db["message_database_status"].find({});
    for(const auto& doc : cursor) {
        register_channel_info(doc, "channel");
    }
    cursor = db["message_database_thread_status"].find({});
    for(const auto& doc : cursor) {
        register_channel_info(doc, "thread");
    }
}

void MessageDatabaseManager::register_channel_info(
    const bsoncxx::v_noabi::document::view &doc,
    std::string_view id_field
) {
    auto public_element = doc["public"];
    ASSERT(public_element.type() == bsoncxx::type::k_bool);
    auto is_public = public_element.get_bool();
    if(!is_public) {
        auto id_element = doc[id_field];
        ASSERT(id_element.type() == bsoncxx::type::k_string);
        auto id = id_element.get_string().value;
        private_channels.insert(id);

        auto name_element = doc["name"];
        ASSERT(name_element.type() == bsoncxx::type::k_string);
        auto name = name_element.get_string().value;
        spdlog::info("private: {} {}", id, name);
    }
}
