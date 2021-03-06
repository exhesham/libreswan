/* Algorithm info parsing and creation functions
 *
 * Author: JuanJo Ciarlante <jjo-ipsec@mendoza.gov.ar>
 * Copyright (C) 2007 Michael Richardson <mcr@xelerance.com>
 * Copyright (C) 2012-2013 Paul Wouters <paul@libreswan.org>
 * Copyright (C) 2013 D. Hugh Redelmeier <hugh@mimosa.com>
 * Copyright (C) 2013 Paul Wouters <pwouters@redhat.com>
 * Copyright (C) 2015-2017 Andrew Cagney
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#ifndef ALG_INFO_H
#define ALG_INFO_H

#include "constants.h"
#include "ike_alg.h"

struct parser_context;
struct alg_info;

/*
 * Parameters to tune the parser.
 */

struct parser_policy {
	bool ikev1;
	bool ikev2;
};

/*
 * Place holder so that it is possible to clearly differentiate
 * between an unspecified rather than 'null' integrity algorithm.
 *
 * NOTE:
 *
 * The callback .alg_info_add() is passed NULL to identify an
 * unspecified algorithm (in keeping with the C tradition of 'no
 * value'), and either 'alg_info_integ_null' or 'ike_alg_encrypt_null'
 * to identify an explicitly specified 'null' algorithm
 *
 * Only when the algorithm is NULL (unspecified or missing) should
 * .alg_info_add() consider adding in defaults.  For instance: in
 * esp=aes, neither the integrity nor DH algorithm were specified so
 * both would be NULL; however in esp=aes_gcm-null, integrity was
 * specified as 'null' so 'ike_alg_integ_null' is used, but DH would
 * still be NULL.
 *
 * 'struct state', on the other hand, uses NULL for 'null' integrity,
 * and 'ike_alg_encrypt_null' for 'null' encryption.  .alg_info_add()
 * must deal with this (for the moment, 'ike_alg_integ_null' should
 * never escape the parser).
 *
 * Why not use NULL for 'null' and something else for unspecifed in
 * the parser?  Several reasons: as noted above, NULL is C's universal
 * identifier of 'no value'; having having lots of ike_alg_XXX_missing
 * structs quickly gets more messy.
 */
extern const struct integ_desc alg_info_integ_null;

struct parser_param {
       const char *protocol;
       enum ike_alg_key ikev1_alg_id;

       /*
        * XXX: Is the proto-id needed?  Parser should be protocol
        * agnostic.
        */
	unsigned protoid;
	/*
	 * If things go wrong, return a non-null error string
	 * (possibly snprintf'd into ERR_BUF).
	 */
	const char *(*alg_info_add)(const struct parser_policy *const policy,
				    struct alg_info *alg_info,
				    const struct encrypt_desc *encrypt,
				    int ealg_id, int ek_bits,
				    int aalg_id,
				    const struct prf_desc *prf,
				    const struct integ_desc *integ,
				    const struct oakley_group_desc *dh_group,
				    char *err_buf, size_t err_buf_len);
	int (*ealg_getbyname)(const char *const str);
	int (*aalg_getbyname)(const char *const str);

	/*
	 * This lookup functions must set err and return null if NAME
	 * isn't valid.
	 */
	const struct ike_alg *(*encrypt_alg_byname)(const struct parser_param *param,
						    const struct parser_policy *const policy,
						    char *err_buf, size_t err_buf_len,
						    const char *name, size_t bit_length);
	const struct ike_alg *(*prf_alg_byname)(const struct parser_param *param,
						const struct parser_policy *const policy,
						char *err_buf, size_t err_buf_len,
						const char *name, size_t key_bit_length);
	const struct ike_alg *(*integ_alg_byname)(const struct parser_param *param,
						  const struct parser_policy *const policy,
						  char *err_buf, size_t err_buf_len,
						  const char *name, size_t key_bit_length);
	const struct ike_alg *(*dh_alg_byname)(const struct parser_param *param,
					       const struct parser_policy *const policy,
					       char *err_buf, size_t err_buf_len,
					       const char *name, size_t key_bit_length);
};

struct esp_info {
	/*
	 * The encryption algorithm and key length; if required by
	 * ESP.
	 *
	 * Because struct encrypt_desc still specifies multiple key
	 * lengths, ENCKEYLEN is still required.
	 */
	const struct encrypt_desc *esp_encrypt;
	u_int8_t transid;       /* enum ipsec_cipher_algo: ESP transform (AES, 3DES, etc.)*/
	u_int32_t enckeylen;    /* keylength for ESP transform (bytes) */
	/*
	 * The authentication algorithm; if required by ESP/AH.
	 */
	const struct integ_desc *esp_integ;
	u_int16_t auth;         /* enum ikev1_auth_attribute: AUTH */
	/*
	 * PFS/DH negotiation.
	 *
	 * XXX: It is called "dh" and not "esp_dh" because "struct
	 * esp_info" and "struct ike_info", which are pretty much
	 * identical, are heading for a merge - one less field to
	 * rename.
	 */
	const struct oakley_group_desc *dh;
};

struct ike_info {
	/*
	 * Encryption.
	 *
	 * Because struct encrypt_desc still specifies multiple key
	 * lengths, ENCKEYLEN is still required.
	 */
	const struct encrypt_desc *ike_encrypt;
	size_t ike_eklen;               /* how many bits required by encryption algo */
	/*
	 * Integrity and PRF.
	 */
	const struct prf_desc *ike_prf;
	const struct integ_desc *ike_integ;
	/*
	 * DH Group
	 */
	const struct oakley_group_desc *ike_dh_group;
};

/* common prefix of struct alg_info_esp and struct alg_info_ike */
struct alg_info {
	int alg_info_cnt;
	int ref_cnt;
	unsigned alg_info_protoid;
};

struct alg_info_esp {
	struct alg_info ai;	/* common prefix */
	struct esp_info esp[128];
	const struct oakley_group_desc *esp_pfsgroup;
};

struct alg_info_ike {
	struct alg_info ai;	/* common prefix */
	struct ike_info ike[128];
};

extern void alg_info_free(struct alg_info *alg_info);
extern void alg_info_addref(struct alg_info *alg_info);
extern void alg_info_delref(struct alg_info *alg_info);

extern struct alg_info_ike *alg_info_ike_create_from_str(lset_t policy,
							 const char *alg_str,
							 char *err_buf, size_t err_buf_len);

extern struct alg_info_esp *alg_info_esp_create_from_str(lset_t policy,
							 const char *alg_str,
							 char *err_buf, size_t err_buf_len);

extern struct alg_info_esp *alg_info_ah_create_from_str(lset_t policy,
							const char *alg_str,
							char *err_buf, size_t err_buf_len);

void alg_info_ike_snprint(char *buf, size_t buflen,
			  const struct alg_info_ike *alg_info_ike);
void alg_info_esp_snprint(char *buf, size_t buflen,
			  const struct alg_info_esp *alg_info_esp);

extern void alg_info_snprint_ike(char *buf, size_t buflen,
			  struct alg_info_ike *alg_info);

void alg_info_snprint_ike_info(char *buf, size_t buflen,
			       struct ike_info *alg_info);

void alg_info_snprint_esp_info(char *buf, size_t buflen,
			       const struct esp_info *esp_info);
void alg_info_snprint_phase2(char *buf, size_t buflen,
			     struct alg_info_esp *alg_info);

/*
 * Iterate through the elements of an ESP or IKE table.
 *
 * Use __typeof__ instead of const to get around ALG_INFO some times
 * being const and sometimes not.
 */

#define FOR_EACH_ESP_INFO(ALG_INFO, ESP_INFO)				\
	for (__typeof__((ALG_INFO)->esp[0]) *(ESP_INFO) = (ALG_INFO)->esp; \
	     (ESP_INFO) < (ALG_INFO)->esp + (ALG_INFO)->ai.alg_info_cnt; \
	     (ESP_INFO)++)

#define FOR_EACH_IKE_INFO(ALG_INFO, IKE_INFO)				\
	for (__typeof__((ALG_INFO)->ike[0]) *(IKE_INFO) = (ALG_INFO)->ike; \
	     (IKE_INFO) < (ALG_INFO)->ike + (ALG_INFO)->ai.alg_info_cnt; \
	     (IKE_INFO)++)

extern int alg_enum_search(enum_names *ed, const char *prefix,
			   const char *postfix, const char *name);

/*
 * on success: returns alg_info
 * on failure: pfree(alg_info) and return NULL;
 *
 * POLICY should be used to guard algorithm supported checks.  For
 * instance: if POLICY=IKEV1, then IKEv1 support is required (IKEv2 is
 * don't care); and if POLICY=IKEV1|IKEV2, then both IKEv1 and IKEv2
 * support is required.
 *
 * Parsing with POLICY=IKEV1, but then proposing the result using
 * IKEv2 is a program error.  The IKEv2 should complain loudly and
 * hopefully not crash.
 *
 * Parsing with POLICY='0' is allowed. It will accept the algorithms
 * unconditionally (spi.c seems to need this).
 */
struct alg_info *alg_info_parse_str(lset_t policy,
				    struct alg_info *alg_info,
				    const char *alg_str,
				    char *err_buf, size_t err_buf_len,
				    const struct parser_param *param);

#endif /* ALG_INFO_H */
