/* config.c - relay backend configuration file routine */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2004 The OpenLDAP Foundation.
 * Portions Copyright 2004 Pierangelo Masarati.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masaratifor inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "back-relay.h"
#include "lutil.h"

int
relay_back_db_config(
    BackendDB	*be,
    const char	*fname,
    int		lineno,
    int		argc,
    char	**argv
)
{
	relay_back_info *ri = (struct relay_back_info *)be->be_private;

	if ( ri == NULL ) {
		fprintf( stderr, "%s: line %d: relay backend info is null!\n",
		    fname, lineno );
		return( 1 );
	}

	/* real naming context */
	if ( strcasecmp( argv[0], "relay" ) == 0 ) {
		struct berval	dn, ndn, pdn;
		int		rc;
		char		*cargv[ 4 ];

		if (argc != 2) {
			fprintf( stderr,
	"%s: line %d: missing relay suffix in \"relay <dn>\" line\n",
			    fname, lineno );
			return( 1 );
		}

		dn.bv_val = argv[ 1 ];
		dn.bv_len = strlen( argv[ 1 ] );
		rc = dnPrettyNormal( NULL, &dn, &pdn, &ndn, NULL );
		if ( rc != LDAP_SUCCESS ) {
			fprintf( stderr, "%s: line %d: "
					"relay dn \"%s\" is invalid\n",
					fname, lineno, argv[ 1 ] );
			return 1;
		}

		ri->ri_bd = select_backend( &ndn, 0, 1 );
		if ( ri->ri_bd == NULL ) {
			fprintf( stderr, "%s: line %d: "
					"cannot find database "
					"of relay dn \"%s\"\n",
					fname, lineno, argv[ 1 ] );
			return 1;

		} else if ( ri->ri_bd == be ) {
			fprintf( stderr, "%s: line %d: "
					"relay dn \"%s\" would call self\n",
					fname, lineno, pdn.bv_val );
			return 1;
		} 

		if ( overlay_config( be, "rewrite-remap" ) ) {
			fprintf( stderr, "unable to install "
					"rewrite-remap overlay "
					"in back-relay \"%s\" => \"%s\"\n",
					be->be_suffix[0].bv_val,
					ri->ri_bd->be_suffix[0].bv_val ? 
					ri->ri_bd->be_suffix[0].bv_val : "<unknown>" );
			return 1;
		}

		cargv[ 0 ] = "suffixmassage";
		cargv[ 1 ] = be->be_suffix[0].bv_val;
		cargv[ 2 ] = ri->ri_bd->be_suffix[0].bv_val;
		cargv[ 3 ] = NULL;

		if ( be->be_config( be, "back-relay", 1, 3, cargv ) ) {
			return 1;
		}

	/* anything else */
	} else {
		return SLAP_CONF_UNKNOWN;
	}

	return 0;
}

