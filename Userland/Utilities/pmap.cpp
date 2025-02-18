/*
 * Copyright (c) 2020, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Assertions.h>
#include <AK/JsonObject.h>
#include <AK/QuickSort.h>
#include <AK/String.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/File.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    if (pledge("stdio rpath", nullptr) < 0) {
        perror("pledge");
        return 1;
    }

    if (unveil("/proc", "r") < 0) {
        perror("unveil");
        return 1;
    }

    unveil(nullptr, nullptr);

    const char* pid;
    static bool extended = false;

    Core::ArgsParser args_parser;
    args_parser.add_option(extended, "Extended output", nullptr, 'x');
    args_parser.add_positional_argument(pid, "PID", "PID", Core::ArgsParser::Required::Yes);
    args_parser.parse(argc, argv);

    auto file = Core::File::construct(String::formatted("/proc/{}/vm", pid));
    if (!file->open(Core::OpenMode::ReadOnly)) {
        warnln("Failed to open {}: {}", file->name(), file->error_string());
        return 1;
    }

    outln("{}:", pid);

    if (extended) {
        outln("Address         Size   Resident      Dirty Access  VMObject Type  Purgeable   CoW Pages Name");
    } else {
        outln("Address         Size Access  Name");
    }

    auto file_contents = file->read_all();
    auto json = JsonValue::from_string(file_contents);
    VERIFY(json.has_value());

    Vector<JsonValue> sorted_regions = json.value().as_array().values();
    quick_sort(sorted_regions, [](auto& a, auto& b) {
        return a.as_object().get("address").to_u64() < b.as_object().get("address").to_u64();
    });

    for (auto& value : sorted_regions) {
        auto& map = value.as_object();
#if ARCH(I386)
        auto address = map.get("address").to_u32();
#else
        auto address = map.get("address").to_u64();
#endif
        auto size = map.get("size").to_string();

        auto access = String::formatted("{}{}{}{}{}",
            (map.get("readable").to_bool() ? "r" : "-"),
            (map.get("writable").to_bool() ? "w" : "-"),
            (map.get("executable").to_bool() ? "x" : "-"),
            (map.get("shared").to_bool() ? "s" : "-"),
            (map.get("syscall").to_bool() ? "c" : "-"));

#if ARCH(I386)
        out("{:08x}  ", address);
#else
        out("{:16x}  ", address);
#endif
        out("{:>10} ", size);
        if (extended) {
            auto resident = map.get("amount_resident").to_string();
            auto dirty = map.get("amount_dirty").to_string();
            auto vmobject = map.get("vmobject").to_string();
            if (vmobject.ends_with("VMObject"))
                vmobject = vmobject.substring(0, vmobject.length() - 8);
            auto purgeable = map.get("purgeable").to_string();
            auto cow_pages = map.get("cow_pages").to_string();
            out("{:>10} ", resident);
            out("{:>10} ", dirty);
            out("{:6} ", access);
            out("{:14} ", vmobject);
            out("{:10} ", purgeable);
            out("{:>10} ", cow_pages);
        } else {
            out("{:6} ", access);
        }
        auto name = map.get("name").to_string();
        out("{:20}", name);
        outln();
    }

    return 0;
}
