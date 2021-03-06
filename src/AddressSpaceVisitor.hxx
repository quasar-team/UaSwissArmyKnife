/*
 * AddressSpaceVisitor.hxx
 *
 *  Created on: Oct 30, 2020
 *      Author: pnikiel
 */

#ifndef SRC_ADDRESSSPACEVISITOR_HXX_
#define SRC_ADDRESSSPACEVISITOR_HXX_

#include <uaexpandednodeid.h>

class AddressSpaceVisitor
{
public:
	virtual void visitingObject (
			const UaExpandedNodeId& id,
			const UaString&         browseName) = 0;

	virtual void visitingVariable (
			const UaExpandedNodeId& id,
			const UaString&         browseName) = 0;

	virtual ~AddressSpaceVisitor() {}
};


#endif /* SRC_ADDRESSSPACEVISITOR_HXX_ */
