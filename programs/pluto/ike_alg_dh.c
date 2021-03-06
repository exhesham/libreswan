/*
 * IKE modular algorithm handling interface, for libreswan
 *
 * Copyright (C) 2016-2017 Andrew Cagney <cagney@gnu.org>
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
#include <stddef.h>
#include <sys/types.h>
#include <libreswan.h>

#include "constants.h"
#include "lswlog.h"
#include "ike_alg.h"

#include "ike_alg_dh.h"
#include "ike_alg_nss_modp.h"
#include "ike_alg_nss_ecp.h"

/*
 * Oakley group description
 *
 * See:
 * RFC-2409 "The Internet key exchange (IKE)" Section 6
 * RFC-3526 "More Modular Exponential (MODP) Diffie-Hellman groups"
 */

/* magic signifier */
const struct oakley_group_desc unset_group = {
	.group = OAKLEY_GROUP_invalid,
};

const struct oakley_group_desc oakley_group_modp1024 = {
	.common = {
		.algo_type = IKE_ALG_DH,
		.name = "MODP1024",
		.names = { "modp1024", "dh2", },
		.officname = "modp1024",
		.id = {
			[IKEv1_OAKLEY_ID] = OAKLEY_GROUP_MODP1024,
			[IKEv1_ESP_ID] = OAKLEY_GROUP_MODP1024,
			[IKEv2_ALG_ID] = OAKLEY_GROUP_MODP1024,
		},
	},
	.group = OAKLEY_GROUP_MODP1024,
	.gen = MODP_GENERATOR,
	.modp = MODP1024_MODULUS,
	.bytes = BYTES_FOR_BITS(1024),
	.dhmke_ops = &ike_alg_nss_modp_dhmke_ops,
};

const struct oakley_group_desc oakley_group_modp1536 = {
	.common = {
		.algo_type = IKE_ALG_DH,
		.name = "MODP1536",
		.names = { "modp1536", "dh5", },
		.officname = "modp1536",
		.id = {
			[IKEv1_OAKLEY_ID] = OAKLEY_GROUP_MODP1536,
			[IKEv1_ESP_ID] = OAKLEY_GROUP_MODP1536,
			[IKEv2_ALG_ID] = OAKLEY_GROUP_MODP1536,
		},
	},
	.group = OAKLEY_GROUP_MODP1536,
	.gen = MODP_GENERATOR,
	.modp = MODP1536_MODULUS,
	.bytes = BYTES_FOR_BITS(1536),
	.dhmke_ops = &ike_alg_nss_modp_dhmke_ops,
};

const struct oakley_group_desc oakley_group_modp2048 = {
	.common = {
		.algo_type = IKE_ALG_DH,
		.name = "MODP2048",
		.names = { "modp2048", "dh14", },
		.officname = "modp2048",
		.id = {
			[IKEv1_OAKLEY_ID] = OAKLEY_GROUP_MODP2048,
			[IKEv1_ESP_ID] = OAKLEY_GROUP_MODP2048,
			[IKEv2_ALG_ID] = OAKLEY_GROUP_MODP2048,
		},
		.fips = TRUE,
	},
	.group = OAKLEY_GROUP_MODP2048,
	.gen = MODP_GENERATOR,
	.modp = MODP2048_MODULUS,
	.bytes = BYTES_FOR_BITS(2048),
	.dhmke_ops = &ike_alg_nss_modp_dhmke_ops,
};

const struct oakley_group_desc oakley_group_modp3072 = {
	.common = {
		.algo_type = IKE_ALG_DH,
		.name = "MODP3072",
		.names = { "modp3072", "dh15", },
		.officname = "modp3072",
		.id = {
			[IKEv1_OAKLEY_ID] = OAKLEY_GROUP_MODP3072,
			[IKEv1_ESP_ID] = OAKLEY_GROUP_MODP3072,
			[IKEv2_ALG_ID] = OAKLEY_GROUP_MODP3072,
		},
		.fips = TRUE,
	},
	.group = OAKLEY_GROUP_MODP3072,
	.gen = MODP_GENERATOR,
	.modp = MODP3072_MODULUS,
	.bytes = BYTES_FOR_BITS(3072),
	.dhmke_ops = &ike_alg_nss_modp_dhmke_ops,
};

const struct oakley_group_desc oakley_group_modp4096 = {
	.common = {
		.algo_type = IKE_ALG_DH,
		.name = "MODP4096",
		.names = { "modp4096", "dh16", },
		.officname = "modp4096",
		.id = {
			[IKEv1_OAKLEY_ID] = OAKLEY_GROUP_MODP4096,
			[IKEv1_ESP_ID] = OAKLEY_GROUP_MODP4096,
			[IKEv2_ALG_ID] = OAKLEY_GROUP_MODP4096,
		},
		.fips = TRUE,
	},
	.group = OAKLEY_GROUP_MODP4096,
	.gen = MODP_GENERATOR,
	.modp = MODP4096_MODULUS,
	.bytes = BYTES_FOR_BITS(4096),
	.dhmke_ops = &ike_alg_nss_modp_dhmke_ops,
};

const struct oakley_group_desc oakley_group_modp6144 = {
	.common = {
		.algo_type = IKE_ALG_DH,
		.name = "MODP6144",
		.names = { "modp6144", "dh17", },
		.officname = "modp6144",
		.id = {
			[IKEv1_OAKLEY_ID] = OAKLEY_GROUP_MODP6144,
			[IKEv1_ESP_ID] = OAKLEY_GROUP_MODP6144,
			[IKEv2_ALG_ID] = OAKLEY_GROUP_MODP6144,
		},
		.fips = TRUE,
	},
	.group = OAKLEY_GROUP_MODP6144,
	.gen = MODP_GENERATOR,
	.modp = MODP6144_MODULUS,
	.bytes = BYTES_FOR_BITS(6144),
	.dhmke_ops = &ike_alg_nss_modp_dhmke_ops,
};

const struct oakley_group_desc oakley_group_modp8192 = {
	.common = {
		.algo_type = IKE_ALG_DH,
		.name = "MODP8192",
		.names = { "modp8192", "dh18", },
		.officname = "modp8192",
		.id = {
			[IKEv1_OAKLEY_ID] = OAKLEY_GROUP_MODP8192,
			[IKEv1_ESP_ID] = OAKLEY_GROUP_MODP8192,
			[IKEv2_ALG_ID] = OAKLEY_GROUP_MODP8192,
		},
		.fips = TRUE,
	},
	.group = OAKLEY_GROUP_MODP8192,
	.gen = MODP_GENERATOR,
	.modp = MODP8192_MODULUS,
	.bytes = BYTES_FOR_BITS(8192),
	.dhmke_ops = &ike_alg_nss_modp_dhmke_ops,
};

const struct oakley_group_desc oakley_group_dh19 = {
	.common = {
		.algo_type = IKE_ALG_DH,
		.name = "DH19",
		.names = { "dh19", "ecp_256", },
		.officname = "dh19",
		.id = {
			[IKEv1_OAKLEY_ID] = OAKLEY_GROUP_ECP_256,
			[IKEv2_ALG_ID] = OAKLEY_GROUP_ECP_256,
		},
		.fips = TRUE,
	},
	.group = OAKLEY_GROUP_ECP_256,
	.bytes = BYTES_FOR_BITS(256) * 2,
	.nss_oid = SEC_OID_SECG_EC_SECP256R1,
	.dhmke_ops = &ike_alg_nss_ecp_dhmke_ops,
};

const struct oakley_group_desc oakley_group_dh20 = {
	.common = {
		.algo_type = IKE_ALG_DH,
		.name = "DH20",
		.names = { "dh20", "ecp_384", },
		.officname = "dh20",
		.id = {
			[IKEv1_OAKLEY_ID] = OAKLEY_GROUP_ECP_384,
			[IKEv2_ALG_ID] = OAKLEY_GROUP_ECP_384,
		},
		.fips = TRUE,
	},
	.group = OAKLEY_GROUP_ECP_384,
	.bytes = BYTES_FOR_BITS(384) * 2,
	.nss_oid = SEC_OID_SECG_EC_SECP384R1,
	.dhmke_ops = &ike_alg_nss_ecp_dhmke_ops,
};

const struct oakley_group_desc oakley_group_dh21 = {
	.common = {
		.algo_type = IKE_ALG_DH,
		.name = "DH21",
		.names = { "dh21", "ecp_521", },
		.officname = "dh21",
		.id = {
			[IKEv1_OAKLEY_ID] = OAKLEY_GROUP_ECP_521,
			[IKEv2_ALG_ID] = OAKLEY_GROUP_ECP_521,
		},
		.fips = TRUE,
	},
	.group = OAKLEY_GROUP_ECP_521,
	.bytes = BYTES_FOR_BITS(521) * 2,
	.nss_oid = SEC_OID_SECG_EC_SECP521R1,
	.dhmke_ops = &ike_alg_nss_ecp_dhmke_ops,
};

#ifdef USE_DH22
const struct oakley_group_desc oakley_group_dh22 = {
	.common = {
		.algo_type = IKE_ALG_DH,
		.name = "DH22",
		.names = { "dh22", },
		.officname = "dh22",
		.id = {
			[IKEv1_OAKLEY_ID] = OAKLEY_GROUP_DH22,
			[IKEv1_ESP_ID] = OAKLEY_GROUP_DH22,
			[IKEv2_ALG_ID] = OAKLEY_GROUP_DH22,
		},
	},
	.group = OAKLEY_GROUP_DH22,
	.gen = MODP_GENERATOR_DH22,
	.modp = MODP1024_MODULUS_DH22,
	.bytes = BYTES_FOR_BITS(1024),
	.dhmke_ops = &ike_alg_nss_modp_dhmke_ops,
};
#endif

const struct oakley_group_desc oakley_group_dh23 = {
	.common = {
		.algo_type = IKE_ALG_DH,
		.name = "DH23",
		.names = { "dh23", },
		.officname = "dh23",
		.id = {
			[IKEv1_OAKLEY_ID] = OAKLEY_GROUP_DH23,
			[IKEv1_ESP_ID] = OAKLEY_GROUP_DH23,
			[IKEv2_ALG_ID] = OAKLEY_GROUP_DH23,
		},
		.fips = TRUE,
	},
	.group = OAKLEY_GROUP_DH23,
	.gen = MODP_GENERATOR_DH23,
	.modp = MODP2048_MODULUS_DH23,
	.bytes = BYTES_FOR_BITS(2048),
	.dhmke_ops = &ike_alg_nss_modp_dhmke_ops,
};

const struct oakley_group_desc oakley_group_dh24 = {
	.common = {
		.algo_type = IKE_ALG_DH,
		.name = "DH24",
		.names = { "dh24", },
		.officname = "dh24",
		.id = {
			[IKEv1_OAKLEY_ID] = OAKLEY_GROUP_DH24,
			[IKEv1_ESP_ID] = OAKLEY_GROUP_DH24,
			[IKEv2_ALG_ID] = OAKLEY_GROUP_DH24,
		},
		.fips = TRUE,
	},
	.group = OAKLEY_GROUP_DH24,
	.gen = MODP_GENERATOR_DH24,
	.modp = MODP2048_MODULUS_DH24,
	.bytes = BYTES_FOR_BITS(2048),
	.dhmke_ops = &ike_alg_nss_modp_dhmke_ops,
};
