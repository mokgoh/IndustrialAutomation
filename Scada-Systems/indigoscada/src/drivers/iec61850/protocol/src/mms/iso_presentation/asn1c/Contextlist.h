/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "ISO8823PRESENTATION"
 * 	found in "../isoPresentationLayer.asn"
 * 	`asn1c -fskeletons-copy`
 */

#ifndef	_Contextlist_H_
#define	_Contextlist_H_


#include <asn_application.h>

/* Including external dependencies */
#include <asn_SEQUENCE_OF.h>
#include "Presentationcontextidentifier.h"
#include "Abstractsyntaxname.h"
#include "Transfersyntaxname.h"
#include <constr_SEQUENCE_OF.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Contextlist */
typedef struct Contextlist {
	A_SEQUENCE_OF(struct Member {
		Presentationcontextidentifier_t	 presentationcontextidentifier;
		Abstractsyntaxname_t	 abstractsyntaxname;
		struct transfersyntaxnamelist {
			A_SEQUENCE_OF(Transfersyntaxname_t) list;
			
			/* Context for parsing across buffer boundaries */
			asn_struct_ctx_t _asn_ctx;
		} transfersyntaxnamelist;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} ) list;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Contextlist_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Contextlist;

#ifdef __cplusplus
}
#endif

#endif	/* _Contextlist_H_ */
