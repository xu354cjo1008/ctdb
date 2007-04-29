/* 
   ctdb utility code

   Copyright (C) Andrew Tridgell  2006

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "includes.h"
#include "lib/events/events.h"
#include "lib/tdb/include/tdb.h"
#include "system/network.h"
#include "system/filesys.h"
#include "../include/ctdb_private.h"

int LogLevel;

/*
  return error string for last error
*/
const char *ctdb_errstr(struct ctdb_context *ctdb)
{
	return ctdb->err_msg;
}


/*
  remember an error message
*/
void ctdb_set_error(struct ctdb_context *ctdb, const char *fmt, ...)
{
	va_list ap;
	talloc_free(ctdb->err_msg);
	va_start(ap, fmt);
	ctdb->err_msg = talloc_vasprintf(ctdb, fmt, ap);
	DEBUG(0,("ctdb error: %s\n", ctdb->err_msg));
	va_end(ap);
}

/*
  a fatal internal error occurred - no hope for recovery
*/
void ctdb_fatal(struct ctdb_context *ctdb, const char *msg)
{
	DEBUG(0,("ctdb fatal error: %s\n", msg));
	fprintf(stderr, "ctdb fatal error: '%s'\n", msg);
	abort();
}

/*
  parse a IP:port pair
*/
int ctdb_parse_address(struct ctdb_context *ctdb,
		       TALLOC_CTX *mem_ctx, const char *str,
		       struct ctdb_address *address)
{
	char *p;
	p = strchr(str, ':');
	if (p == NULL) {
		ctdb_set_error(ctdb, "Badly formed node '%s'\n", str);
		return -1;
	}
	
	address->address = talloc_strndup(mem_ctx, str, p-str);
	address->port = strtoul(p+1, NULL, 0);
	return 0;
}


/*
  check if two addresses are the same
*/
bool ctdb_same_address(struct ctdb_address *a1, struct ctdb_address *a2)
{
	return strcmp(a1->address, a2->address) == 0 && a1->port == a2->port;
}


/*
  hash function for mapping data to a VNN - taken from tdb
*/
uint32_t ctdb_hash(const TDB_DATA *key)
{
	uint32_t value;	/* Used to compute the hash value.  */
	uint32_t i;	/* Used to cycle through random values. */

	/* Set the initial value from the key size. */
	for (value = 0x238F13AF * key->dsize, i=0; i < key->dsize; i++)
		value = (value + (key->dptr[i] << (i*5 % 24)));

	return (1103515243 * value + 12345);  
}

/*
  a type checking varient of idr_find
 */
static void *_idr_find_type(struct idr_context *idp, int id, const char *type, const char *location)
{
	void *p = idr_find(idp, id);
	if (p && talloc_check_name(p, type) == NULL) {
		DEBUG(0,("%s idr_find_type expected type %s  but got %s\n",
			 location, type, talloc_get_name(p)));
		return NULL;
	}
	return p;
}


/*
  update a max latency number
 */
void ctdb_latency(double *latency, struct timeval t)
{
	double l = timeval_elapsed(&t);
	if (l > *latency) {
		*latency = l;
	}
}

#if 0
struct idr_fake {
	uint32_t size;
	void **ptrs;
};

static void idr_fake_init(struct ctdb_context *ctdb)
{
	if (ctdb->fidr) return;
	ctdb->fidr = talloc(ctdb, struct idr_fake);
	ctdb->fidr->size = 0x10000;
	ctdb->fidr->ptrs = talloc_zero_array(ctdb->fidr, void *, 
						 ctdb->fidr->size);
}

uint32_t ctdb_reqid_new(struct ctdb_context *ctdb, void *state)
{
	uint32_t i;
	idr_fake_init(ctdb);
	for (i=0;i<ctdb->fidr->size;i++) {
		if (ctdb->fidr->ptrs[i] == NULL) {
			ctdb->fidr->ptrs[i] = state;
			return i;
		}
	}
	return (uint32_t)-1;
}

void *_ctdb_reqid_find(struct ctdb_context *ctdb, uint32_t reqid, const char *type, const char *location)
{
	idr_fake_init(ctdb);
	if (ctdb->fidr->ptrs[reqid] == NULL) {
		DEBUG(0,("bad fidr id %u\n", reqid));
	}
	return ctdb->fidr->ptrs[reqid];
}


void ctdb_reqid_remove(struct ctdb_context *ctdb, uint32_t reqid)
{
	idr_fake_init(ctdb);
	ctdb->fidr->ptrs[reqid] = NULL;
}

#else
uint32_t ctdb_reqid_new(struct ctdb_context *ctdb, void *state)
{
	uint32_t id;

	id  = ctdb->idr_cnt++ & 0xFFFF;
	id |= (idr_get_new(ctdb->idr, state, 0xFFFF)<<16);
	return id;
}

void *_ctdb_reqid_find(struct ctdb_context *ctdb, uint32_t reqid, const char *type, const char *location)
{
	void *p;

	p = _idr_find_type(ctdb->idr, (reqid>>16)&0xFFFF, type, location);
	if (p == NULL) {
		DEBUG(0, ("Could not find idr:%d\n",reqid));
	}

	return p;
}


void ctdb_reqid_remove(struct ctdb_context *ctdb, uint32_t reqid)
{
	int ret;

	ret = idr_remove(ctdb->idr, (reqid>>16)&0xFFFF);
	if (ret != 0) {
		DEBUG(0, ("Removing idr that does not exist\n"));
	}
}

#endif
