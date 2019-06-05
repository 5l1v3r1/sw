// Copyright (C) 2016-2019 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "target.h"

namespace sw
{

ITarget::~ITarget() = default;

TargetEntryPoint::~TargetEntryPoint() = default;

void TargetData::loadPackages(const PackageIdSet &whitelist)
{
    if (!ep)
        throw SW_RUNTIME_ERROR("No entry point provided");
    ep->loadPackages(whitelist);
}

void TargetData::setEntryPoint(std::unique_ptr<TargetEntryPoint> e)
{
    ep = std::move(e);
}

TargetData::Base::iterator TargetData::find(const TargetSettings &s)
{
    return std::find_if(begin(), end(), [&s](const auto &t)
    {
        return *t == s;
    });
}

TargetData::Base::const_iterator TargetData::find(const TargetSettings &s) const
{
    return std::find_if(begin(), end(), [&s](const auto &t)
    {
        return *t == s;
    });
}

}
