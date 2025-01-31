/*
 *  mms_client_named_variable_list.c
 *
 *  MMS named variable list services (client)
 *
 *  Copyright 2013 Michael Zillgith
 *
 *	This file is part of libIEC61850.
 *
 *	libIEC61850 is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	libIEC61850 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	See COPYING file for the complete license text.
 */
// Modified by Enscada limited http://www.enscada.com
#include <string.h>

#include <MmsPdu.h>
#include "stack_config.h"
#include "mms_common.h"
#include "mms_client_connection.h"
#include "byte_buffer.h"
#include "string_utilities.h"
#include "mms_client_internal.h"


void
mmsClient_createDeleteNamedVariableListRequest(long invokeId, ByteBuffer* writeBuffer,
		char* domainId, char* listNameId)
{
	asn_enc_rval_t rval;

	DeleteNamedVariableListRequest_t* request;
	MmsPdu_t* mmsPdu = mmsClient_createConfirmedRequestPdu(invokeId);

	mmsPdu->choice.confirmedRequestPdu.confirmedServiceRequest.present =
				ConfirmedServiceRequest_PR_deleteNamedVariableList;

	request =
			&(mmsPdu->choice.confirmedRequestPdu.confirmedServiceRequest.choice.deleteNamedVariableList);

	request->listOfVariableListName = calloc(1,
			sizeof(struct DeleteNamedVariableListRequest__listOfVariableListName));

	request->listOfVariableListName->list.count = 1;
	request->listOfVariableListName->list.size = 1;

	request->listOfVariableListName->list.array = calloc(1, sizeof(ObjectName_t*));
	request->listOfVariableListName->list.array[0] = calloc(1, sizeof(ObjectName_t));

	request->listOfVariableListName->list.array[0]->present = ObjectName_PR_domainspecific;
	request->listOfVariableListName->list.array[0]->choice.domainspecific.domainId.size = strlen(domainId);
	request->listOfVariableListName->list.array[0]->choice.domainspecific.domainId.buf = copyString(domainId);
	request->listOfVariableListName->list.array[0]->choice.domainspecific.itemId.size = strlen(listNameId);
	request->listOfVariableListName->list.array[0]->choice.domainspecific.itemId.buf = copyString(listNameId);

	request->scopeOfDelete = calloc(1, sizeof(INTEGER_t));
	asn_long2INTEGER(request->scopeOfDelete, DeleteNamedVariableListRequest__scopeOfDelete_specific);

	rval = der_encode(&asn_DEF_MmsPdu, mmsPdu,
	            mmsClient_write_out, (void*) writeBuffer);

	if (DEBUG) xer_fprint(stdout, &asn_DEF_MmsPdu, mmsPdu);

	asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);
}

void
mmsClient_createDeleteAssociationSpecificNamedVariableListRequest(
		long invokeId,
		ByteBuffer* writeBuffer,
		char* listNameId)
{
	asn_enc_rval_t rval;
	DeleteNamedVariableListRequest_t* request;
	MmsPdu_t* mmsPdu = mmsClient_createConfirmedRequestPdu(invokeId);

	mmsPdu->choice.confirmedRequestPdu.confirmedServiceRequest.present =
				ConfirmedServiceRequest_PR_deleteNamedVariableList;

	request =
			&(mmsPdu->choice.confirmedRequestPdu.confirmedServiceRequest.choice.deleteNamedVariableList);

	request->listOfVariableListName = calloc(1,
			sizeof(struct DeleteNamedVariableListRequest__listOfVariableListName));

	request->listOfVariableListName->list.count = 1;
	request->listOfVariableListName->list.size = 1;

	request->listOfVariableListName->list.array = calloc(1, sizeof(ObjectName_t*));
	request->listOfVariableListName->list.array[0] = calloc(1, sizeof(ObjectName_t));

	request->listOfVariableListName->list.array[0]->present = ObjectName_PR_aaspecific;

	request->listOfVariableListName->list.array[0]->choice.aaspecific.size = strlen(listNameId);
	request->listOfVariableListName->list.array[0]->choice.aaspecific.buf = copyString(listNameId);

	request->scopeOfDelete = calloc(1, sizeof(INTEGER_t));
	asn_long2INTEGER(request->scopeOfDelete, DeleteNamedVariableListRequest__scopeOfDelete_specific);

	rval = der_encode(&asn_DEF_MmsPdu, mmsPdu,
	            mmsClient_write_out, (void*) writeBuffer);

	if (DEBUG) xer_fprint(stdout, &asn_DEF_MmsPdu, mmsPdu);

	asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);
}

MmsIndication
mmsClient_parseDeleteNamedVariableListResponse(ByteBuffer* message, uint32_t* invokeId)
{
	MmsPdu_t* mmsPdu = 0;

	MmsIndication indication = MMS_ERROR;

	asn_dec_rval_t rval;

	rval = ber_decode(NULL, &asn_DEF_MmsPdu,
			(void**) &mmsPdu, ByteBuffer_getBuffer(message), ByteBuffer_getSize(message));


	if (rval.code == RC_OK) {
		if (mmsPdu->present == MmsPdu_PR_confirmedResponsePdu) {
			*invokeId = mmsClient_getInvokeId(&mmsPdu->choice.confirmedResponsePdu);

			if (mmsPdu->choice.confirmedResponsePdu.confirmedServiceResponse.present ==
					ConfirmedServiceResponse_PR_deleteNamedVariableList)
			{
				DeleteNamedVariableListResponse_t* response =
					&(mmsPdu->choice.confirmedResponsePdu.confirmedServiceResponse.choice.deleteNamedVariableList);

				long numberDeleted;

				asn_INTEGER2long(&(response->numberDeleted), &numberDeleted);

				if (numberDeleted == 1)
					indication = MMS_OK;
			}
		}
	}

	asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);

	return indication;
}


void
mmsClient_createGetNamedVariableListAttributesRequest(uint32_t invokeId, ByteBuffer* writeBuffer,
		char* domainId, char* listNameId)
{
	GetNamedVariableListAttributesRequest_t* request;
	MmsPdu_t* mmsPdu = mmsClient_createConfirmedRequestPdu(invokeId);

	mmsPdu->choice.confirmedRequestPdu.confirmedServiceRequest.present =
				ConfirmedServiceRequest_PR_getNamedVariableListAttributes;

	request =
			&(mmsPdu->choice.confirmedRequestPdu.confirmedServiceRequest.choice.getNamedVariableListAttributes);

	request->present = ObjectName_PR_domainspecific;

	request->choice.domainspecific.domainId.size = strlen(domainId);
	request->choice.domainspecific.domainId.buf = copyString(domainId);

	request->choice.domainspecific.itemId.size = strlen(listNameId);
	request->choice.domainspecific.itemId.buf = copyString(listNameId);

	der_encode(&asn_DEF_MmsPdu, mmsPdu,
			mmsClient_write_out, (void*) writeBuffer);

	if (DEBUG) xer_fprint(stdout, &asn_DEF_MmsPdu, mmsPdu);

	asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);
}

static LinkedList
parseNamedVariableAttributes(GetNamedVariableListAttributesResponse_t* response, bool* deletable)
{
	LinkedList attributes;
	int attributesCount;
	int i;
	
	if (deletable != NULL)
		*deletable = response->mmsDeletable;

	attributesCount = response->listOfVariable.list.count;
	
	attributes = LinkedList_create();

	for (i = 0; i < attributesCount; i++) {
		//TODO add checks

		char* itemName = mmsMsg_createStringFromAsnIdentifier(response->listOfVariable.list.array[i]->
				variableSpecification.choice.name.choice.domainspecific.itemId);

		LinkedList_add(attributes, itemName);
	}

	return attributes;
}

LinkedList
mmsClient_parseGetNamedVariableListAttributesResponse(ByteBuffer* message, uint32_t* invokeId,
		bool* /*OUT*/ deletable)
{
	MmsPdu_t* mmsPdu = 0;

	LinkedList attributes = NULL;

	asn_dec_rval_t rval;

	rval = ber_decode(NULL, &asn_DEF_MmsPdu,
			(void**) &mmsPdu, ByteBuffer_getBuffer(message), ByteBuffer_getSize(message));


	if (rval.code == RC_OK) {
		if (mmsPdu->present == MmsPdu_PR_confirmedResponsePdu) {
			*invokeId = mmsClient_getInvokeId(&mmsPdu->choice.confirmedResponsePdu);

			if (mmsPdu->choice.confirmedResponsePdu.confirmedServiceResponse.present ==
					ConfirmedServiceResponse_PR_getNamedVariableListAttributes)
			{
				attributes = parseNamedVariableAttributes(
						&(mmsPdu->choice.confirmedResponsePdu.confirmedServiceResponse.choice.getNamedVariableListAttributes),
						deletable);
			}
		}
	}

	asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);

	return attributes;
}


void
mmsClient_createDefineNamedVariableListRequest(
		uint32_t invokeId,
		ByteBuffer* writeBuffer,
		char* domainId,
		char* listNameId,
		LinkedList /*<MmsVariableSpecification*>*/ listOfVariables,
		bool associationSpecific)
{
	LinkedList element;
	int i;
	int listSize;
	DefineNamedVariableListRequest_t* request;
	MmsPdu_t* mmsPdu = mmsClient_createConfirmedRequestPdu(invokeId);

	mmsPdu->choice.confirmedRequestPdu.confirmedServiceRequest.present =
				ConfirmedServiceRequest_PR_defineNamedVariableList;

	request =
		&(mmsPdu->choice.confirmedRequestPdu.confirmedServiceRequest.choice.defineNamedVariableList);

	if (associationSpecific) {
		request->variableListName.present = ObjectName_PR_aaspecific;

		request->variableListName.choice.aaspecific.size = strlen(listNameId);
		request->variableListName.choice.aaspecific.buf = copyString(listNameId);
	}
	else {
		request->variableListName.present = ObjectName_PR_domainspecific;

		request->variableListName.choice.domainspecific.domainId.size = strlen(domainId);
		request->variableListName.choice.domainspecific.domainId.buf = copyString(domainId);

		request->variableListName.choice.domainspecific.itemId.size = strlen(listNameId);
		request->variableListName.choice.domainspecific.itemId.buf = copyString(listNameId);
	}

	listSize = LinkedList_size(listOfVariables);

	request->listOfVariable.list.count = listSize;
	request->listOfVariable.list.size = listSize;

	request->listOfVariable.list.array = calloc(listSize, sizeof(void*));

	i = 0;
	element = LinkedList_getNext(listOfVariables);
	while (i < listSize) {

		MmsVariableSpecification* variableSpec = (MmsVariableSpecification*) element->data;

		request->listOfVariable.list.array[i] =
				calloc(1, sizeof(struct DefineNamedVariableListRequest__listOfVariable__Member));

		request->listOfVariable.list.array[i]->variableSpecification.present =
				VariableSpecification_PR_name;

		request->listOfVariable.list.array[i]->variableSpecification.choice.name.present =
				ObjectName_PR_domainspecific;

		request->listOfVariable.list.array[i]->variableSpecification.choice.name.choice.
			domainspecific.domainId.size = strlen(variableSpec->domainId);

		request->listOfVariable.list.array[i]->variableSpecification.choice.name.choice.
			domainspecific.domainId.buf = copyString(variableSpec->domainId);

		request->listOfVariable.list.array[i]->variableSpecification.choice.name.choice.
			domainspecific.itemId.size = strlen(variableSpec->itemId);

		request->listOfVariable.list.array[i]->variableSpecification.choice.name.choice.
			domainspecific.itemId.buf = copyString(variableSpec->itemId);

		element = LinkedList_getNext(element);

		i++;
	}

	der_encode(&asn_DEF_MmsPdu, mmsPdu,
			mmsClient_write_out, (void*) writeBuffer);

	if (DEBUG) xer_fprint(stdout, &asn_DEF_MmsPdu, mmsPdu);

	asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);
}

MmsIndication
mmsClient_parseDefineNamedVariableResponse(ByteBuffer* message, uint32_t* invokeId)
{
	MmsPdu_t* mmsPdu = 0;
	MmsIndication retVal =  MMS_ERROR;

	MmsValue* valueList = NULL;
	MmsValue* value = NULL;

	asn_dec_rval_t rval;

	rval = ber_decode(NULL, &asn_DEF_MmsPdu,
			(void**) &mmsPdu, ByteBuffer_getBuffer(message), ByteBuffer_getSize(message));


	if (rval.code == RC_OK) {
		if (mmsPdu->present == MmsPdu_PR_confirmedResponsePdu) {
			*invokeId = mmsClient_getInvokeId(&mmsPdu->choice.confirmedResponsePdu);

			if (mmsPdu->choice.confirmedResponsePdu.confirmedServiceResponse.present ==
					ConfirmedServiceResponse_PR_defineNamedVariableList)
				retVal = MMS_OK;
		}
	}

	asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);

	return retVal;
}
