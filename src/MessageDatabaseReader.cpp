#include "MessageDatabaseReader.hpp"

#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/uri.hpp>
#include <libassert/assert.hpp>

#include "utils.hpp"
#include "constants.hpp"

MessageDatabaseReader::MessageDatabaseReader(mongocxx::cursor cursor, const string_set& private_channels)
    : cursor(std::move(cursor)),
        private_channels(private_channels),
        read_thread(&MessageDatabaseReader::reader, this) {}

[[gnu::noinline]] std::optional<MessageDatabaseEntry> MessageDatabaseReader::read() {
    while(!queue.front()) {}
    auto entry = std::move(*queue.front());
    queue.pop();
    return entry;
}

MessageDatabaseEntry MessageDatabaseReader::parse_document(const bsoncxx::document::view &doc) {
    auto timestamp_element = doc["timestamp"];
    ASSERT(timestamp_element.type() == bsoncxx::type::k_double);
    auto timestamp_ms = timestamp_element.get_double();
    sys_ms timestamp{std::chrono::milliseconds(timestamp_ms)};

    auto edits_element = doc["edits"];
    ASSERT(edits_element.type() == bsoncxx::type::k_array);
    bsoncxx::array::view edits_array = edits_element.get_array();
    ASSERT(!edits_array.empty());
    auto last_edit_element = *last(edits_array.begin(), edits_array.end());
    auto content_element = last_edit_element["content"];
    ASSERT(content_element.type() == bsoncxx::type::k_string);
    auto content_boost_sv = content_element.get_string().value;
    std::string_view content{content_boost_sv.begin(), content_boost_sv.end()};

    return MessageDatabaseEntry{timestamp, std::string(content)};
}

void MessageDatabaseReader::reader() {
    for(const auto& doc : cursor) {
        auto id_element = doc["channel"];
        ASSERT(id_element.type() == bsoncxx::type::k_string);
        auto id = id_element.get_string();
        if(is_bot_id(id.value)) {
            continue;
        }

        // auto channel_element = doc["channel"];
        // ASSERT(channel_element.type() == bsoncxx::type::k_string);
        // auto channel_id = channel_element.get_string();
        // if(private_channels.contains(channel_id.value)) {
        //     continue;
        // }

        queue.emplace(parse_document(doc));

        #ifdef TRACE
        if(count++ == 100'000) {
            trace_trigger();
            std::exit(0);
        }
        #endif
    }
    queue.emplace(std::nullopt);
}
