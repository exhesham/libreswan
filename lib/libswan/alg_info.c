/*
 * Algorithm info parsing and creation functions
 * Author: JuanJo Ciarlante <jjo-ipsec@mendoza.gov.ar>
 *
 * Copyright (C) 2012 Paul Wouters <paul@libreswan.org>
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

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "lswlog.h"
#include "lswalloc.h"
#include "constants.h"
#include "alg_info.h"
#include "ike_alg.h"

const struct integ_desc alg_info_integ_null = {
       .common = {
               .name = "XXX-NULL-XXX",
	       .algo_type = IKE_ALG_INTEG,
       },
};

/*
 *	Creates a new alg_info by parsing passed string
 */
enum parser_state {
	ST_INI_EA,      /* parse ike= or esp= string */
	ST_INI_AA,      /* parse ah= string */
	ST_EA,          /* encrypt algo   */
	ST_EA_END,
	ST_EK,          /* enc. key length */
	ST_EK_END,
	ST_AA,          /* auth algo */
	ST_AA_END,
	ST_MODP,        /* modp spec */
	ST_END,
	ST_EOF,
};

/* XXX:jjo to implement different parser for ESP and IKE */
struct parser_context {
	unsigned state;
	const struct parser_param *param;
	struct parser_policy policy;
	char ealg_buf[20];
	char eklen_buf[20];
	char aalg_buf[20];
	char modp_buf[20];
	char *ealg_str;
	char *eklen_str;
	char *aalg_str;
	char *modp_str;
	int eklen;
	int ch;	/* character that stopped parsing */
};

#define MAX_ALG_ALIASES 16

struct alg_alias {
	const char *const alg;
	const char *const alias_set[MAX_ALG_ALIASES];
};

/* if str is a known alias, return the real alg */
static const char *find_alg_alias(const struct alg_alias *alias, const char *str)
{
	const struct alg_alias *aa;

	for (aa = alias; aa->alg != NULL; aa++) {
		const char *const *aset;

		for (aset = aa->alias_set; *aset != NULL; aset++) {
			if (strcaseeq(str, *aset)) {
				return aa->alg;
			}
		}
	}
	return NULL;
}

static int alg_getbyname_or_alias(const struct alg_alias *aliases, const char *str,
				  int (*getbyname)(const char *const str))
{
	const char *alias = find_alg_alias(aliases, str);

	return getbyname(alias == NULL ? str : alias);
}

static int aalg_getbyname_or_alias(const struct parser_context *context,
				   const char *str)
{
	static const struct alg_alias aliases[] = {
		/* alg */	/* aliases */
		{ "sha2_256",	{ "sha2", NULL } },
		{ "sha2_256",	{ "sha256", NULL } },
		{ "sha2_384",	{ "sha384", NULL } },
		{ "sha2_512",	{ "sha512", NULL } },
		{ "sha1",	{ "sha", NULL } },
		{ "sha1",	{ "sha1_96", NULL } },
		{ "aes_cmac_96", { "aes_cmac", NULL } },
		{ NULL, { NULL } }
	};

	return alg_getbyname_or_alias(aliases, str, context->param->aalg_getbyname);
}

/*
 * Aliases should NOT be used to match a base cipher to a key size,
 * as that would change the meaning of the loaded connection. For
 * examples aes cannot become an alias for aes128 or else a responder
 * with esp=aes would reject aes256.
 */

static int ealg_getbyname_or_alias(const struct parser_context *context,
				   const char *str)
{
	static const struct alg_alias aliases[] = {
		/* alg */	/* aliases */
		{ "aes_ccm_a",	{ "aes_ccm_8",  NULL } },
		{ "aes_ccm_b",	{ "aes_ccm_12", NULL } },
		{ "aes_ccm_c",	{ "aes_ccm_16", "aes_ccm", NULL } },
		{ "aes_gcm_a",	{ "aes_gcm_8", NULL } },
		{ "aes_gcm_b",	{ "aes_gcm_12", NULL } },
		{ "aes_gcm_c",	{ "aes_gcm_16", "aes_gcm", NULL } },
		{ "aes_ctr",	{ "aesctr", NULL } },
		{ "aes",	{ "aes_cbc", NULL } },
		{ NULL, { NULL } }
	};

	return alg_getbyname_or_alias(aliases, str, context->param->ealg_getbyname);
}

/*
 * Search enum_name array with string, uppercased, prefixed, and postfixed
 */
int alg_enum_search(enum_names *ed, const char *prefix,
		    const char *postfix, const char *name)
{
	char buf[64];
	size_t prelen = strlen(prefix);
	size_t postlen = strlen(postfix);
	size_t name_len = strlen(name);

	if (prelen + name_len + postlen >= sizeof(buf))
		return -1;	/* cannot match */

	memcpy(buf, prefix, prelen);
	memcpy(buf + prelen, name, name_len);
	memcpy(buf + prelen + name_len, postfix, postlen + 1);	/* incl. NUL */

	return enum_search(ed, buf);
}

static const char *parser_state_names[] = {
	"ST_INI_EA",
	"ST_INI_AA",
	"ST_EA",
	"ST_EA_END",
	"ST_EK",
	"ST_EK_END",
	"ST_AA",
	"ST_AA_END",
	"ST_MOPD",
	"ST_END",
	"ST_EOF",
};

static const char *parser_state_name(enum parser_state state)
{
	passert(state < elemsof(parser_state_names));
	return parser_state_names[state];
}

static inline void parser_set_state(struct parser_context *p_ctx,
				    enum parser_state state)
{
	p_ctx->state = state;
}

static void parser_init(struct parser_context *ctx,
			lset_t policy,
			const struct parser_param *param)
{
	*ctx = (struct parser_context) {
		.param = param,
		.policy = {
			.ikev1 = LIN(POLICY_IKEV1_ALLOW, policy),
			/*
			 * logic needs to match pick_initiator()
			 *
			 * XXX: Once pluto is changed to IKEv1 XOR
			 * IKEv2 it should be possible to move this
			 * magic into pluto proper and instead pass a
			 * simple boolean.
			 */
			.ikev2 = ((policy & POLICY_IKEV2_PROPOSE)
				  && (policy & POLICY_IKEV2_ALLOW)),
		 },
		.state = (param->ealg_getbyname || param->encrypt_alg_byname
			  ? ST_INI_EA
			  : ST_INI_AA),
		/*
		 * DANGER: this is a pointer to a very small buffer on
		 * the stack.
		 */
		.ealg_str = ctx->ealg_buf,
		.eklen_str = ctx->eklen_buf,
		.aalg_str = ctx->aalg_buf,
		.modp_str = ctx->modp_buf,
	};
}

static err_t parser_machine(struct parser_context *p_ctx)
{
	int ch = p_ctx->ch;

	/* chars that end algo strings */
	switch (ch) {
	case '\0':		/* end-of-string */
	case ',':	/* algo string separator */
		switch (p_ctx->state) {
		case ST_EA:
		case ST_EK:
		case ST_AA:
		case ST_MODP:
		{
			enum parser_state next_state = 0;

			switch (ch) {
			case '\0':
				next_state = ST_EOF;
				break;
			case ',':
				next_state = ST_END;
				break;
			}
			parser_set_state(p_ctx, next_state);
			return NULL;
		}
		default:
			return "String ended with invalid char";
		}
	}

	for (;;) {
		DBG(DBG_CONTROLMORE,
		    DBG_log("state=%s ealg_buf='%s' eklen_buf='%s' aalg_buf='%s' modp_buf='%s'",
			    parser_state_name(p_ctx->state),
			    p_ctx->ealg_buf,
			    p_ctx->eklen_buf,
			    p_ctx->aalg_buf,
			    p_ctx->modp_buf));
		/*
		 * There are three ways out of this switch:
		 * - break: successful termination of the function
		 * - return diag: unsuccessful termination of the function
		 * - continue: repeat the switch
		 */
		switch (p_ctx->state) {
		case ST_INI_EA:
			if (isspace(ch))
				break;
			if (isalnum(ch)) {
				*(p_ctx->ealg_str++) = ch;
				parser_set_state(p_ctx, ST_EA);
				break;
			}
			return "No alphanum. char initially found";

		case ST_INI_AA:
			if (isspace(ch))
				break;
			if (isalnum(ch)) {
				*(p_ctx->aalg_str++) = ch;
				parser_set_state(p_ctx, ST_AA);
				break;
			}
			return "No alphanum. char initially found";

		case ST_EA:
			if (isalnum(ch) || ch == '_') {
				/*
				 * accept all of <ealg>[_<eklen>]
				 */
				*(p_ctx->ealg_str++) = ch;
				break;
			}
			if (ch == '-') {
				*(p_ctx->ealg_str++) = '\0';
				parser_set_state(p_ctx, ST_EA_END);
				break;
			}
			return "No valid char found after enc alg string";

		case ST_EA_END:
			if (isdigit(ch)) {
				/*
				 * Given legacy <ealg>-<eklen>, save
				 * <eklen>.
				 */
				parser_set_state(p_ctx, ST_EK);
				continue;
			}
			if (isalpha(ch)) {
				parser_set_state(p_ctx, ST_AA);
				continue;
			}
			return "No alphanum char found after enc alg separator";

		case ST_EK:
			if (isdigit(ch)) {
				*(p_ctx->eklen_str++) = ch;
				break;
			}
			if (ch == '-') {
				*(p_ctx->eklen_str++) = '\0';
				parser_set_state(p_ctx, ST_EK_END);
				break;
			}
			return "Non digit or valid separator found while reading enc keylen";

		case ST_EK_END:
			if (isalpha(ch)) {
				parser_set_state(p_ctx, ST_AA);
				continue;
			}
			return "Non alpha char found after enc keylen end separator";

		case ST_AA:
			if (ch == ';' || ch == '-') {
				*(p_ctx->aalg_str++) = 0;
				parser_set_state(p_ctx, ST_AA_END);
				break;
			}
			if (isalnum(ch) || ch == '_') {
				*(p_ctx->aalg_str++) = ch;
				break;
			}
			return "Non alphanum or valid separator found in auth string";

		case ST_AA_END:
			/*
			 * Only allow modpXXXX string if we have
			 * a modp_getbyname method
			 */
			if (p_ctx->param->dh_alg_byname != NULL && isalpha(ch)) {
				parser_set_state(p_ctx, ST_MODP);
				continue;
			}
			return "Invalid modulus";

		case ST_MODP:
			if (isalnum(ch) || ch == '_') {
				*(p_ctx->modp_str++) = ch;
				break;
			}
			return "Non alphanum char found after in modp string";

		case ST_END:
		case ST_EOF:
			break;
		}
		return NULL;
	}
}

static const struct ike_alg *lookup_byname(struct parser_context *p_ctx,
					   char *err_buf, size_t err_buf_len,
					   const struct ike_alg *(alg_byname)(const struct parser_param *param,
									      const struct parser_policy *const policy,
									      char *err_buf, size_t err_buf_len,
									      const char *name,
									      size_t key_bit_length),
					   const char *name,
					   size_t key_bit_length,
					   const char *what)
{
	err_buf[0] = '\0';
	if (name[0] != '\0') {
		if (alg_byname != NULL) {
			const struct ike_alg *alg = alg_byname(p_ctx->param,
							       &p_ctx->policy,
							       err_buf, err_buf_len,
							       name, key_bit_length);
			if (alg == NULL) {
				DBG(DBG_CONTROLMORE,
				    DBG_log("%s_byname('%s') failed: %s",
					    what, name, err_buf));
				passert(err_buf[0]);
				return NULL;
			}
			DBG(DBG_CONTROLMORE,
			    DBG_log("%s_byname('%s') returned '%s'",
				    what, name, alg->name));
			return alg;
		} else {
			DBG(DBG_CONTROLMORE,
			    DBG_log("ignoring %s '%s'", what, name));
			return NULL;
		}
	}
	return NULL;
}

static int parse_eklen(char *err_buf, size_t err_buf_len,
			const char *eklen_buf)
{
	/* convert -<eklen> if present */
	long eklen = strtol(eklen_buf, NULL, 10);
	if (eklen >= INT_MAX) {
		snprintf(err_buf, err_buf_len,
			 "encryption key length WAY too big");
		return 0;
	}
	if (eklen == 0) {
		snprintf(err_buf, err_buf_len,
			 "encryption key length is zero");
		return 0;
	}
	return eklen;
}

static const char *parser_alg_info_add(struct parser_context *p_ctx,
				       char *err_buf, size_t err_buf_len,
				       struct alg_info *alg_info)
{
	DBG(DBG_CONTROLMORE,
	    DBG_log("add ealg_buf='%s' eklen_buf='%s' aalg_buf='%s' modp_buf='%s'",
		    p_ctx->ealg_buf,
		    p_ctx->eklen_buf,
		    p_ctx->aalg_buf,
		    p_ctx->modp_buf));

	/*
	 * Try the raw EALG string with "-<eklen>" if present.
	 * Strings like aes_gcm_16 and aes_gcm_16_256 end up in
	 * <ealg>, while strings like aes_gcm_16-256 end up in
	 * <ealg>-<eklen>.
	 */
	if (p_ctx->eklen_buf[0] != '\0') {
		/* convert -<eklen> if present */
		p_ctx->eklen = parse_eklen(err_buf, err_buf_len, p_ctx->eklen_buf);
		if (p_ctx->eklen <= 0) {
			passert(err_buf[0] != '\0');
			return err_buf;
		}
	}
	const struct encrypt_desc *encrypt =
		encrypt_desc(lookup_byname(p_ctx, err_buf, err_buf_len,
					   p_ctx->param->encrypt_alg_byname,
					   p_ctx->ealg_buf, p_ctx->eklen,
					   "encryption"));
	if (err_buf[0] != '\0') {
		if (p_ctx->eklen_buf[0] != '\0') {
			/* <ealg>-<eklen> was rejected */
			return err_buf;
		}
		char *end = &p_ctx->ealg_buf[strlen(p_ctx->ealg_buf) > 0 ?  strlen(p_ctx->ealg_buf) - 1 : 0];
		if (!isdigit(*end)) {
			/* <eklen> was rejected */
			return err_buf;
		}
		/*
		 * Trailing digit so assume that <ealg> is really
		 * <ealg>_<eklen> or <ealg><eklen>, strip of the
		 * <eklen> and try again.
		 */
		do {
			if (end == p_ctx->ealg_buf) {
				/* <ealg> missing */
				return err_buf;
			}
			end--;
		} while (isdigit(*end));
		p_ctx->eklen = parse_eklen(err_buf, err_buf_len, end + 1);
		if (p_ctx->eklen <= 0) {
			passert(err_buf[0] != '\0');
			return err_buf;
		}
		/*
		 * strip optional "_" when "<ealg>_<eklen>"
		 */
		if (end > p_ctx->ealg_buf && *end == '_') {
			end--;
		}
		/* truncate and try again */
		end[1] = '\0';
		err_buf[0] = '\0';
		encrypt = encrypt_desc(lookup_byname(p_ctx, err_buf, err_buf_len,
						     p_ctx->param->encrypt_alg_byname,
						     p_ctx->ealg_buf, p_ctx->eklen,
						     "encryption"));
		if (err_buf[0] != '\0') {
			return err_buf;
		}
	}

#	define COMMON_KEY_LENGTH(x) ((x) == 0 || (x) == 128 || (x) == 192 || (x) == 256)

	int ealg_id = -1;
	if (p_ctx->param->ealg_getbyname && p_ctx->ealg_buf[0] != '\0') {
		ealg_id = ealg_getbyname_or_alias(p_ctx, p_ctx->ealg_buf);
		DBG(DBG_CONTROLMORE, DBG_log("ealg_id = %d", ealg_id));
		if (ealg_id < 0) {
			return "enc_alg not found";
		}

		/* reject things we know but don't like */
		switch (p_ctx->param->protoid) {
		case PROTO_ISAKMP:
			switch (ealg_id) {
			case OAKLEY_DES_CBC:
			case OAKLEY_IDEA_CBC:
			case OAKLEY_BLOWFISH_CBC:
			case OAKLEY_RC5_R16_B64_CBC:
				return "IKE cipher not implemented";
			}
			break;
		case PROTO_IPSEC_ESP:
			switch (ealg_id) {
			case ESP_reserved:
			case ESP_DES_IV64:
			case ESP_DES:
			case ESP_RC5:
			case ESP_IDEA:
			case ESP_BLOWFISH:
			case ESP_3IDEA:
			case ESP_DES_IV32:
			case ESP_RC4:
			case ESP_ID17:
				/*
				 * kernel uses IKEv1, where it is camellia
				 * - case ESP_RESERVED_FOR_IEEE_P1619_XTS_AES:
				 */
				return "ESP cipher not implemented";
			}
			break;
		}

		/*
		 * Enforce RFC restrictions in key size, documented in
		 * ietf_constants.h
		 * If using --impair-send-key-size-check this check is bypassed
		 * for testing purposes.
		 */
		if (p_ctx->eklen != 0 && !DBGP(IMPAIR_SEND_KEY_SIZE_CHECK)) {
			switch (p_ctx->param->protoid) {
			case PROTO_ISAKMP:
				switch (ealg_id) {
				case OAKLEY_3DES_CBC:
					return "3DES does not take variable key lengths";
				case OAKLEY_CAST_CBC:
					if (p_ctx->eklen != 128) {
						return "CAST is only supported for 128 bits (to avoid padding)";
					}
					break;
				case OAKLEY_SERPENT_CBC:
				case OAKLEY_TWOFISH_CBC:
				case OAKLEY_TWOFISH_CBC_SSH:
				case OAKLEY_AES_CBC:
				case OAKLEY_AES_CTR:
				case OAKLEY_AES_GCM_8:
				case OAKLEY_AES_GCM_12:
				case OAKLEY_AES_GCM_16:
				case OAKLEY_AES_CCM_8:
				case OAKLEY_AES_CCM_12:
				case OAKLEY_AES_CCM_16:
				case OAKLEY_CAMELLIA_CBC:
				case OAKLEY_CAMELLIA_CTR:
				case OAKLEY_CAMELLIA_CCM_A:
				case OAKLEY_CAMELLIA_CCM_B:
				case OAKLEY_CAMELLIA_CCM_C:
					if (!COMMON_KEY_LENGTH(p_ctx->eklen)) {
						return "wrong encryption key length - key size must be 128 (default), 192 or 256";
					}
					break;
				}
				break;
			case PROTO_IPSEC_ESP:
				switch (ealg_id) {
				case ESP_3DES:
					return "3DES does not take variable key lengths";
				case ESP_NULL:
					return "NULL does not take variable key lengths";
				case ESP_CAST:
					if (!COMMON_KEY_LENGTH(p_ctx->eklen)) {
						return "CAST is only supported for 128 bits (to avoid padding)";
					}
					break;
				case ESP_CAMELLIAv1: /* this value is hit instead */
				case ESP_AES:
				case ESP_AES_CTR:
				case ESP_AES_GCM_8:
				case ESP_AES_GCM_12:
				case ESP_AES_GCM_16:
				case ESP_AES_CCM_8:
				case ESP_AES_CCM_12:
				case ESP_AES_CCM_16:
				case ESP_TWOFISH:
				case ESP_SERPENT:
					if (!COMMON_KEY_LENGTH(p_ctx->eklen)) {
						return "wrong encryption key length - key size must be 128 (default), 192 or 256";
					}
					break;
#if 0
				case ESP_SEED_CBC:
					if (p_ctx->eklen != 128) {
						return "wrong encryption key length - SEED-CBC key size must be 128";
					}
					break;
#endif
				}
				break;
			}
		}
	}

	const struct prf_desc *prf =
		prf_desc(lookup_byname(p_ctx, err_buf, err_buf_len,
				       p_ctx->param->prf_alg_byname,
				       p_ctx->aalg_buf, 0,
				       "PRF"));
	if (err_buf[0] != '\0') {
		return err_buf;
	}

	const struct integ_desc *integ =
		integ_desc(lookup_byname(p_ctx, err_buf, err_buf_len,
					 p_ctx->param->integ_alg_byname,
					 p_ctx->aalg_buf, 0,
					 "integrity"));
	if (err_buf[0] != '\0') {
		return err_buf;
	}

	int aalg_id = -1;
	if (p_ctx->param->aalg_getbyname && *p_ctx->aalg_buf != '\0') {
		aalg_id = aalg_getbyname_or_alias(p_ctx, p_ctx->aalg_buf);
		DBG(DBG_CONTROLMORE, DBG_log("aalg_id = %d", aalg_id));
		if (aalg_id < 0) {
			return "hash_alg not found";
		}

		/* some code stupidly uses INT_MAX for "null" */
		if (aalg_id == AH_NONE || aalg_id == AH_NULL || aalg_id == INT_MAX) {
			switch (p_ctx->param->protoid) {
			case PROTO_IPSEC_ESP:
				/*
				 * ESP AEAD ciphers do not require
				 * separate authentication (by
				 * definition, authentication is
				 * built-in).
				 */
				switch (ealg_id) {
				case ESP_AES_GCM_8:
				case ESP_AES_GCM_12:
				case ESP_AES_GCM_16:
				case ESP_AES_CCM_8:
				case ESP_AES_CCM_12:
				case ESP_AES_CCM_16:
					break; /* ok */
				default:
					return "non-AEAD ESP cipher cannot have null authentication";
				}
				break;
			case PROTO_ISAKMP:
				/*
				 * While IKE AEAD ciphers do not
				 * require separate authentication (by
				 * definition, authentication is
				 * built-in), they do require a PRF.
				 *
				 * The non-empty authentication
				 * algorithm will be used as the PRF.
				 */
				switch (ealg_id) {
				case OAKLEY_AES_CCM_8:
				case OAKLEY_AES_CCM_12:
				case OAKLEY_AES_CCM_16:
				case OAKLEY_AES_GCM_8:
				case OAKLEY_AES_GCM_12:
				case OAKLEY_AES_GCM_16:
				case OAKLEY_CAMELLIA_CCM_A:
				case OAKLEY_CAMELLIA_CCM_B:
				case OAKLEY_CAMELLIA_CCM_C:
					return "AEAD IKE cipher cannot have null pseudo-random-function";
				default:
					return "non-AEAD IKE cipher cannot have null authentication";
				}
				break;
			case PROTO_IPSEC_AH:
				return "AH cannot have null authentication";
			}
		} else {
			switch (p_ctx->param->protoid) {
			case PROTO_IPSEC_ESP:
				/*
				 * ESP AEAD ciphers do not require
				 * separate authentication (by
				 * definition, authentication is
				 * built-in).
				 *
				 * Reject any non-null authentication
				 * algorithm
				 */
				switch (ealg_id) {
				case ESP_AES_GCM_8:
				case ESP_AES_GCM_12:
				case ESP_AES_GCM_16:
				case ESP_AES_CCM_8:
				case ESP_AES_CCM_12:
				case ESP_AES_CCM_16:
					return "AEAD ESP cipher must have null authentication";
				default:
					break; /* ok */
				}
				break;
			case PROTO_ISAKMP:
				/*
				 * While IKE AEAD ciphers do not
				 * require separate authentication (by
				 * definition, authentication is
				 * built-in), they do require a PRF.
				 *
				 * So regardless of the algorithm type
				 * allow an explicit authentication.
				 * (IKE AEAD uses it for the PRF).
				 */
				break;
			}
		}

		if (!DBGP(IMPAIR_SEND_KEY_SIZE_CHECK)) {
			switch (aalg_id) {
			case AH_NULL:
				if (ealg_id == -1)
					return "Encryption and authentication cannot both be null";
				break;
			default:
				break;
			}
		}
	}

	const struct oakley_group_desc *group =
		oakley_group_desc(lookup_byname(p_ctx, err_buf, err_buf_len,
						p_ctx->param->dh_alg_byname,
						p_ctx->modp_buf, 0,
						"group"));
	if (err_buf[0] != '\0') {
		return err_buf;
	}

	return p_ctx->param->alg_info_add(&p_ctx->policy,
					  alg_info,
					  encrypt,
					  ealg_id, p_ctx->eklen,
					  aalg_id,
					  prf, integ,
					  group,
					  err_buf, err_buf_len);

#	undef COMMON_KEY_LENGTH
}

/*
 * on success: returns alg_info
 * on failure: alg_info_free(alg_info) and return NULL;
 */
struct alg_info *alg_info_parse_str(lset_t policy,
				    struct alg_info *alg_info,
				    const char *alg_str,
				    char *err_buf, size_t err_buf_len,
				    const struct parser_param *param)
{
	DBG(DBG_CONTROL,
	    DBG_log("parsing '%s' for %s", alg_str, param->protocol));

	struct parser_context ctx;
	int ret;
	const char *ptr;

	alg_info->alg_info_protoid = param->protoid;
	err_buf[0] = '\0';

	parser_init(&ctx, policy, param);

	/* use default if null string */
	if (*alg_str == '\0') {
		param->alg_info_add(&ctx.policy, alg_info,
				    NULL,
				    0, 0, 0,
				    NULL, NULL, NULL,
				    err_buf, err_buf_len);
		return alg_info;
	}

	ptr = alg_str;
	do {
		ctx.ch = *ptr++;
		{
			err_t pm_ugh = parser_machine(&ctx);
			if (pm_ugh != NULL) {
				snprintf(err_buf, err_buf_len,
					 "%s, just after \"%.*s\" (state=%s)",
					 pm_ugh,
					 (int)(ptr - alg_str - 1), alg_str,
					 parser_state_name(ctx.state));
				alg_info_free(alg_info);
				return NULL;
			}
		}
		ret = ctx.state;
		switch (ret) {
		case ST_END:
		case ST_EOF:
			{
				char error[100] = ""; /* arbitrary */
				err_t ugh = parser_alg_info_add(&ctx, error, sizeof(error),
								alg_info);
				if (ugh != NULL) {
					snprintf(err_buf, err_buf_len,
						"%s, enc_alg=\"%s\"(%d), auth_alg=\"%s\", modp=\"%s\"",
						ugh, ctx.ealg_buf, ctx.eklen, ctx.aalg_buf,
						ctx.modp_buf);
					alg_info_free(alg_info);
					return NULL;
				}
			}
			/* zero out for next run (ST_END) */
			parser_init(&ctx, policy, param);
			break;

		default:
			/* ??? this is nonsense: in either case, break will happen */
			if (ctx.ch != '\0')
				break;
		}
	} while (ret < ST_EOF);
	return alg_info;
}

/*
 * alg_info struct can be shared by several connections instances,
 * handle free() with ref_cnts.
 *
 * Use alg_info_free() if the value returned by *_parse_str() is found
 * to be (semantically) bogus.
 */

void alg_info_free(struct alg_info *alg_info)
{
	passert(alg_info);
	passert(alg_info->ref_cnt == 0);
	pfree(alg_info);
}

void alg_info_addref(struct alg_info *alg_info)
{
	alg_info->ref_cnt++;
}

void alg_info_delref(struct alg_info *alg_info)
{
	passert(alg_info->ref_cnt != 0);
	alg_info->ref_cnt--;
	if (alg_info->ref_cnt == 0)
		alg_info_free(alg_info);
}
