/*
 * =====================================================================================
 *
 *       Filename:  rt.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/22/2020 06:14:33 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Er. Abhishek Sagar, Juniper Networks (https://csepracticals.wixsite.com/csepracticals), sachinites@gmail.com
 *        Company:  Juniper Networks
 *
 *        This file is part of the Netlink Sockets distribution (https://github.com/sachinites) 
 *        Copyright (c) 2019 Abhishek Sagar.
 *        This program is free software: you can redistribute it and/or modify it under the terms of the GNU General 
 *        Public License as published by the Free Software Foundation, version 3.
 *        
 *        This program is distributed in the hope that it will be useful, but
 *        WITHOUT ANY WARRANTY; without even the implied warranty of
 *        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *        General Public License for more details.
 *
 *        visit website : https://csepracticals.wixsite.com/csepracticals for more courses and projects
 *                                  
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include "rt.h"

void
rt_init_rt_table(rt_table_t *rt_table){

    rt_table->head = NULL;
}

rt_entry_t *
rt_add_or_update_rt_entry(rt_table_t *rt_table,
                    char *dest, 
                    char mask, 
                    char *gw_ip, 
                    char *oif){

	bool new_entry;
	bool rt_entry_has_data;
    rt_entry_t *head = NULL;
    rt_entry_t *rt_entry = NULL;	
	new_entry = false;

	rt_entry_has_data = false;

	rt_entry = rt_look_up_rt_entry(rt_table, dest, mask);

	if(!rt_entry) {
    	
		rt_entry = calloc(1, sizeof(rt_entry_t));
    	
		strncpy(rt_entry->rt_entry_keys.dest, dest,
			sizeof(rt_entry->rt_entry_keys.dest));
    	rt_entry->rt_entry_keys.mask = mask;
		
		rt_entry->nfc = nfc_create_new_notif_chain(0);
		rt_entry->created = false;
	}

    if(gw_ip)
        strncpy(rt_entry->gw_ip, gw_ip, sizeof(rt_entry->gw_ip));
    if(oif)
        strncpy(rt_entry->oif, oif, sizeof(rt_entry->oif));

	if (gw_ip || oif) {

		rt_entry_has_data = true;
	}

    head = rt_table->head;
    rt_table->head = rt_entry;
    rt_entry->prev = 0;
    rt_entry->next = head;
    if(head)
    head->prev = rt_entry;

	nfc_op_t nfc_op_code = NFC_UNKNOWN;

	if (rt_entry->created == false &&
			rt_entry_has_data) {

		/* Publisher has just created this rt_entry */
		nfc_op_code = NFC_ADD;
		rt_entry->created = true;	
	}
	else{
		/*  Publisher has updated the already existing rt_entry */
		nfc_op_code = NFC_MOD;
	}

	if (nfc_op_code == NFC_ADD || 
		nfc_op_code == NFC_MOD) {
		nfc_invoke_notif_chain(rt_entry->nfc,
				(char *)rt_entry, sizeof(rt_entry_t),
				0, 0, nfc_op_code);
	}
	
    return rt_entry;
}

bool
rt_delete_rt_entry(rt_table_t *rt_table,
    char *dest, char mask){

    rt_entry_t *rt_entry = NULL;

    ITERTAE_RT_TABLE_BEGIN(rt_table, rt_entry){
    
        if(strncmp(rt_entry->rt_entry_keys.dest, 
            dest, sizeof(rt_entry->rt_entry_keys.dest)) == 0 &&
            rt_entry->rt_entry_keys.mask == mask){

            rt_entry_remove(rt_table, rt_entry);
			nfc_invoke_notif_chain(rt_entry->nfc,
				(char *)rt_entry, sizeof(rt_entry_t),
				0, 0, NFC_DEL);
			free(rt_entry->nfc);
            free(rt_entry);
            return true;
        }
    } ITERTAE_RT_TABLE_END(rt_table, curr);

    return false;
}

void
rt_clear_rt_table(rt_table_t *rt_table){


}

void
rt_free_rt_table(rt_table_t *rt_table){


}

void
rt_dump_rt_table(rt_table_t *rt_table){

    rt_entry_t *rt_entry = NULL;

    ITERTAE_RT_TABLE_BEGIN(rt_table, rt_entry){

        printf("%-20s %-4d %-20s %s\n",
            rt_entry->rt_entry_keys.dest, 
            rt_entry->rt_entry_keys.mask, 
            rt_entry->gw_ip,
            rt_entry->oif);
		printf("\tPrinting Subscribers : ");
		
		glthread_t *curr;
		notif_chain_elem_t *nfce;

		ITERATE_GLTHREAD_BEGIN(&rt_entry->nfc->notif_chain_head, curr){

			nfce = glthread_glue_to_notif_chain_elem(curr);
			
			printf("%u ", nfce->subs_id);

		} ITERATE_GLTHREAD_END(&rt_entry->nfc->notif_chain_head, curr)

    } ITERTAE_RT_TABLE_END(rt_table, rt_entry);
}

rt_entry_t *
rt_look_up_rt_entry(rt_table_t *rt_table,
					char *dest, char mask) {

	rt_entry_t *rt_entry = NULL;
	
	ITERTAE_RT_TABLE_BEGIN(rt_table, rt_entry) {

		if ((strncmp(rt_entry->rt_entry_keys.dest,
					dest, sizeof(rt_entry->rt_entry_keys.dest)) == 0) &&
			rt_entry->rt_entry_keys.mask == mask) {

			return rt_entry;
		}
	} ITERTAE_RT_TABLE_END(rt_table, rt_entry);
	return NULL;
}

void
rt_table_register_for_notification(
		rt_table_t *rt_table,
		rt_entry_keys_t *key,
		size_t key_size,
		nfc_app_cb app_cb,
		uint32_t subs_id) {

	bool new_entry_created;
	rt_entry_t *rt_entry;
	notif_chain_elem_t nfce;

	new_entry_created = false;

	rt_entry = rt_look_up_rt_entry(rt_table, key->dest, key->mask);

	if (!rt_entry) {
		/* rt_entry was not existing before, but we are
 		 * creating it because subscriber is interested in notif
 		 * for this entry. Create such an entry without data. Later
 		 * When publisher would actually cate this entry, all registered
 		 * subscriber should be notified
 		 * */	
		rt_entry = rt_add_or_update_rt_entry(
					rt_table, key->dest, key->mask, 0, 0);
		new_entry_created = true;
	}
	
	memset(&nfce, 0, sizeof(notif_chain_elem_t));

	assert(key_size <= MAX_NOTIF_KEY_SIZE);

	/* No need to keep keys as nfce;s are tied to
	 * routing table entry which contain keys*/	
	//memcpy(nfce.key, (char *)key, key_size);
	//nfce.key_size = key_size;
	nfce.app_cb = app_cb;
	nfce.subs_id = subs_id;
	nfc_register_notif_chain(rt_entry->nfc, &nfce);

	/* Subscriber subscribes to already existing rt entry,
 	 * immediately send notif to Subscriber with opcode
 	 * NFC_ADD*/
	if (!new_entry_created) {
		app_cb(rt_entry, sizeof(rt_entry_t), NFC_ADD, subs_id);
	}
}

