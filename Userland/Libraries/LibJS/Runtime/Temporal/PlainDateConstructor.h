/*
 * Copyright (c) 2021, Idan Horowitz <idan.horowitz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibJS/Runtime/NativeFunction.h>

namespace JS::Temporal {

class PlainDateConstructor final : public NativeFunction {
    JS_OBJECT(PlainDateConstructor, NativeFunction);

public:
    explicit PlainDateConstructor(GlobalObject&);
    virtual void initialize(GlobalObject&) override;
    virtual ~PlainDateConstructor() override = default;

    virtual Value call() override;
    virtual Value construct(FunctionObject& new_target) override;

private:
    virtual bool has_constructor() const override { return true; }
};

}
