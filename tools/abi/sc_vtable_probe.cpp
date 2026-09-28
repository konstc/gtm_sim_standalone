// SPDX-FileCopyrightText: 2026 Konstantin Chernyshov
// SPDX-License-Identifier: Apache-2.0
// Instantiates SystemC/TLM templates that also appear (as weak vtables) in the
// prebuilt GTM libraries, so their vtable layouts can be compared with
// tools/abi/vtable_compare.py.
#include <systemc>
#include <tlm>
#include <tlm_utils/peq_with_get.h>

using namespace sc_core;
using sc_dt::sc_bv;
typedef tlm::tlm_base_protocol_types P;

template class sc_core::sc_signal<bool>;
template class sc_core::sc_signal<sc_bv<5> >;
template class sc_core::sc_signal<sc_bv<8> >;
template class sc_core::sc_signal<unsigned long>;
template class sc_core::sc_signal<long>;
template class sc_core::sc_in<sc_bv<2> >;
template class sc_core::sc_in<sc_bv<8> >;
template class sc_core::sc_in<sc_bv<16> >;
template class sc_core::sc_in<unsigned long>;
template class sc_core::sc_inout<sc_bv<8> >;
template class sc_core::sc_inout<sc_bv<1024> >;
template class sc_core::sc_out<bool>;
template class sc_core::sc_out<sc_bv<8> >;
template class sc_core::sc_out<unsigned long>;
template class sc_core::sc_port_b<sc_signal_in_if<bool> >;
template class sc_core::sc_port_b<sc_signal_in_if<long> >;
template class sc_core::sc_port_b<sc_signal_inout_if<sc_bv<8> > >;
template class sc_core::sc_event_finder_t<sc_signal_in_if<bool> >;
template class sc_core::sc_export<tlm::tlm_fw_transport_if<P> >;
template class sc_core::sc_export<tlm::tlm_bw_transport_if<P> >;
template class sc_core::sc_port<tlm::tlm_bw_transport_if<P>, 1, SC_ONE_OR_MORE_BOUND>;
template class tlm::tlm_base_target_socket<32, tlm::tlm_fw_transport_if<P>, tlm::tlm_bw_transport_if<P>, 1, SC_ONE_OR_MORE_BOUND>;
template class tlm_utils::peq_with_get<tlm::tlm_generic_payload>;

// Non-template classes whose vtables the libraries emit as well.
sc_vector<sc_in<bool> >* probe_vector() { return new sc_vector<sc_in<bool> >("v", 2); }
tlm::tlm_endian_context* probe_endian() { return new tlm::tlm_endian_context; }
