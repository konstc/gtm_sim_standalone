// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// cos_compat - deferred, size-propagating vector binding (see header).
#include "sca_basic_libraries/generic_tb_utilities/cos_connectivity_elaborator.h"

#include <map>
#include <memory>
#include <vector>

namespace sca_basic_libraries_namespace
{

static_assert(sizeof(cos_connectivity_elaborator) == 16,
              "cos_connectivity_elaborator must keep the COSIDE object size");

namespace
{

/** A binding between two objects together with its elaboration function. */
struct pending_binding
{
    cos_connectivity_elaborator::elaboration_function func;
    const sc_core::sc_object* objects[2];
    bool done;
};

typedef std::shared_ptr<pending_binding> binding_ptr;

/** Object -> bindings the object participates in. */
std::multimap<const sc_core::sc_object*, std::weak_ptr<pending_binding> >& binding_registry()
{
    static std::multimap<const sc_core::sc_object*, std::weak_ptr<pending_binding> > registry;
    return registry;
}

void run_once(const binding_ptr& binding, cos_connectivity_elaborator::elaboration_data_base& data)
{
    if (!binding || binding->done || !binding->func) return;
    binding->done = true;  // before the call: the function may recurse into us
    binding->func(data);
}

void unregister(const binding_ptr& binding)
{
    auto& registry = binding_registry();
    for (const sc_core::sc_object* obj : binding->objects)
    {
        auto range = registry.equal_range(obj);
        for (auto it = range.first; it != range.second;)
        {
            if (it->second.expired() || it->second.lock() == binding)
                it = registry.erase(it);
            else
                ++it;
        }
    }
}

} // namespace

struct cos_connectivity_elaborator::impl
{
    elaboration_function next_function;
    std::vector<binding_ptr> bindings;
};

cos_connectivity_elaborator::cos_connectivity_elaborator()
    : m_impl(new impl), m_reserved(nullptr)
{
}

cos_connectivity_elaborator::~cos_connectivity_elaborator()
{
    for (const binding_ptr& binding : m_impl->bindings) unregister(binding);
    delete m_impl;
}

void cos_connectivity_elaborator::set_elaboration_function(elaboration_function func)
{
    m_impl->next_function = std::move(func);
}

void cos_connectivity_elaborator::bind(sc_core::sc_object& obj, sc_core::sc_object& bound_obj)
{
    binding_ptr binding = std::make_shared<pending_binding>();
    binding->func = std::move(m_impl->next_function);
    binding->objects[0] = &obj;
    binding->objects[1] = &bound_obj;
    binding->done = false;
    m_impl->next_function = nullptr;
    m_impl->bindings.push_back(binding);

    binding_registry().emplace(&obj, binding);
    if (&bound_obj != &obj) binding_registry().emplace(&bound_obj, binding);
}

void cos_connectivity_elaborator::elaborate(elaboration_data_base& data)
{
    if (!m_impl->bindings.empty()) run_once(m_impl->bindings.back(), data);
}

void cos_connectivity_elaborator::elaborate(elaboration_data_base& data, sc_core::sc_object& obj)
{
    // Copy first: the elaboration functions create/bind further objects.
    std::vector<binding_ptr> affected;
    auto range = binding_registry().equal_range(&obj);
    for (auto it = range.first; it != range.second; ++it)
    {
        if (binding_ptr binding = it->second.lock()) affected.push_back(binding);
    }
    for (const binding_ptr& binding : affected) run_once(binding, data);
}

} // namespace sca_basic_libraries_namespace
