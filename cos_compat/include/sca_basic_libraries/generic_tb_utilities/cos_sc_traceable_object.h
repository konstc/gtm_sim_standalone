// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// cos_compat - replacement for the COSIDE traceable-object interface.
//
// Objects implementing this interface can be traced by name through
// sc_object_trace(). ABI: pure interface (vptr only) with the virtual
// functions in this order: trace_sc, destructor.
#ifndef COS_COMPAT_COS_SC_TRACEABLE_OBJECT_H_
#define COS_COMPAT_COS_SC_TRACEABLE_OBJECT_H_

#include <systemc>
#include <string>

namespace sca_basic_libraries_namespace
{

class cos_sc_traceable_object
{
public:
    virtual void trace_sc(sc_core::sc_trace_file* tf, const std::string& nm) const = 0;
    virtual ~cos_sc_traceable_object() {}
};

} // namespace sca_basic_libraries_namespace

#endif // COS_COMPAT_COS_SC_TRACEABLE_OBJECT_H_
