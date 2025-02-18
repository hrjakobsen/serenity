/*
 * Copyright (c) 2021, Idan Horowitz <idan.horowitz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibJS/Runtime/Object.h>

namespace JS::Temporal {

class PlainDatePrototype final : public Object {
    JS_OBJECT(PlainDatePrototype, Object);

public:
    explicit PlainDatePrototype(GlobalObject&);
    virtual void initialize(GlobalObject&) override;
    virtual ~PlainDatePrototype() override = default;

private:
    JS_DECLARE_NATIVE_FUNCTION(calendar_getter);
};

}
