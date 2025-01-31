/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "MMS"
 * 	found in "../mms-extended.asn"
 * 	`asn1c -fskeletons-copy`
 */

#ifndef	_InitiateResponsePdu_H_
#define	_InitiateResponsePdu_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Integer32.h"
#include "Integer16.h"
#include "Integer8.h"
#include "InitResponseDetail.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* InitiateResponsePdu */
typedef struct InitiateResponsePdu {
	Integer32_t	*localDetailCalled	/* OPTIONAL */;
	Integer16_t	 negotiatedMaxServOutstandingCalling;
	Integer16_t	 negotiatedMaxServOutstandingCalled;
	Integer8_t	*negotiatedDataStructureNestingLevel	/* OPTIONAL */;
	InitResponseDetail_t	 mmsInitResponseDetail;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} InitiateResponsePdu_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_InitiateResponsePdu;

#ifdef __cplusplus
}
#endif

#endif	/* _InitiateResponsePdu_H_ */
