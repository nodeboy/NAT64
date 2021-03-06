/*
 * send_fragmented_packets.c
 *
 *  Created on: Oct 3, 2013
 *      Author: user
 */


#include <linux/module.h>
#include <linux/slab.h>

#include "nat64/unit/unit_test.h"
#include "nat64/unit/skb_generator.h"
#include "nat64/unit/validator.h"
#include "nat64/unit/types.h"
#include "nat64/comm/str_utils.h"
#include "nat64/comm/constants.h"
#include "nat64/mod/ipv6_hdr_iterator.h"
#include "nat64/mod/send_packet.h"
/* *#include "nat64/mod/send_packet_old.h" */

#define GENERATE_FOR_EACH true
#include "determine_incoming_tuple.c"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roberto Aceves");
MODULE_AUTHOR("Alberto Leiva");
MODULE_DESCRIPTION("Packet database test");

#define DST_ADDR "10.2.18.230"
#define DST_PORT 80
#define SRC_ADDR "192.168.1.3"
#define SRC_PORT 5000

static bool send_packet(struct sk_buff *skb)
{
	struct dst_entry *dst;
	int error;

	dst = route_ipv4(ip_hdr(skb), udp_hdr(skb), L4PROTO_UDP, 0);
	if (!dst)
		return false;
	skb->dev = dst->dev;
	skb_dst_set(skb, dst);

	error = ip_local_out(skb);
	if (error) {
		log_err(ERR_SEND_FAILED, "The kernel's packet dispatch function returned errcode %d. "
				"Cannot send packet.", -error);
		return false;
	}

	return true;
}

/**
 *
 */
static bool test_send_ipv4_fragment_first(void)
{
	struct sk_buff *skb1;
	struct iphdr *hdr4;
	struct ipv4_pair pair4;
	int error;

	error = init_pair4(&pair4, SRC_ADDR, SRC_PORT, DST_ADDR, DST_PORT);
	if (error)
		return false;

	error = create_skb_ipv4_udp(&pair4, &skb1, 100);
	if (error)
		return false;

	/* Set IPv4 network header */
	hdr4 = ip_hdr(skb1);
	hdr4->frag_off = build_ipv4_frag_off_field(false, true, 0);
	hdr4->check = 0;
	hdr4->check = ip_fast_csum(hdr4, hdr4->ihl);

	/* Actually send the packet */
	return send_packet(skb1);
}

static bool test_send_ipv4_fragment_middle(void)
{
	struct sk_buff *skb1;
	struct iphdr *hdr4;
	struct ipv4_pair pair4;
	int error;

	error = init_pair4(&pair4, SRC_ADDR, SRC_PORT, DST_ADDR, DST_PORT);
	if (error)
		return false;

	error = create_skb_ipv4_udp_fragment(&pair4, &skb1, 100);
	if (error)
		return false;

	/* Set IPv4 network header */
	hdr4 = ip_hdr(skb1);
	hdr4->frag_off = build_ipv4_frag_off_field(false, true, 100 + sizeof(struct udphdr));
	hdr4->check = 0;
	hdr4->check = ip_fast_csum(hdr4, hdr4->ihl);

	/* Actually send the packet */
	return send_packet(skb1);
}

static bool test_send_ipv4_fragment_last(void)
{
	struct sk_buff *skb1;
	struct iphdr *hdr4;
	struct ipv4_pair pair4;
	int error;

	error = init_pair4(&pair4, SRC_ADDR, SRC_PORT, DST_ADDR, DST_PORT);
	if (error)
		return false;

	error = create_skb_ipv4_udp_fragment(&pair4, &skb1, 100);
	if (error)
		return false;

	/* Set IPv4 network header */
	hdr4 = ip_hdr(skb1);
	hdr4->frag_off = build_ipv4_frag_off_field(false, false, 200 + sizeof(struct udphdr));
	hdr4->check = 0;
	hdr4->check = ip_fast_csum(hdr4, hdr4->ihl);

	/* Actually send the packet */
	return send_packet(skb1);
}

int init_module(void)
{
	START_TESTS("Determine incoming tuple");


	CALL_TEST(test_send_ipv4_fragment_first(), "Send just the first fragment, which includes transport header, of an IPv4 packet.");
	CALL_TEST(test_send_ipv4_fragment_middle(), "Send just the middle fragment, which NOT includes transport header, of an IPv4 packet.");
	CALL_TEST(test_send_ipv4_fragment_last(), "Send just the last fragment, which NOT includes transport header, of an IPv4 packet.");


	END_TESTS;
}

void cleanup_module(void)
{
	/* No code. */
}
