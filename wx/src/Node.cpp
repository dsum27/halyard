// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// @BEGIN_LICENSE
//
// Halyard - Multimedia authoring and playback system
// Copyright 1993-2009 Trustees of Dartmouth College
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// @END_LICENSE

#include "AppHeaders.h"
#include <boost/foreach.hpp>
#include "Node.h"
#include "Element.h"
#include "HalyardApp.h"
#include "Stage.h"
#include "StageFrame.h"
#include "ElementsPane.h"

using namespace Halyard;


//=========================================================================
//  Constructor and destructor
//=========================================================================

int Node::sNodeCount = 0;

Node::Node(const wxString &inName, Halyard::TCallbackPtr inDispatcher)
    : mName(inName), mLogName(inName.mb_str())
{
    ASSERT(mName != wxT(""));

    // Increment the number of nodes which currently exist.
    sNodeCount++;

    // If we're not the root node, look up our parent node.
    if (mName != wxT("/")) {
        size_t last_slash(mName.rfind(wxT("/")));
        wxString parent_name;
        if (last_slash == 0)
            parent_name = wxT("/");
        else
            parent_name = mName.substr(0, last_slash);
        mParent = wxGetApp().GetStage()->FindNode(parent_name);
        ASSERT(mParent);
    }

    if (inDispatcher) {
        // Initialize a named pointer on a separate line to prevent leaks.
        // We should do this pretty much everywhere we use shared_ptr.
        EventDispatcherPtr ptr(new EventDispatcher());
        mEventDispatcher = ptr;
        mEventDispatcher->SetDispatcher(inDispatcher);
    }
}

Node::~Node() {
    // Decrement the number of nodes which exist.
    ASSERT(sNodeCount >= 1);
    sNodeCount--;
}


//=========================================================================
//  Name
//=========================================================================

wxString Node::GetDisplayName(bool *outIsAnonymous) {
    wxString result;
    bool is_anonymous = false;
    if (IsRootNode()) {
        result = mName;
    } else {
        size_t last_slash(mName.rfind(wxT("/")));
        result = mName.substr(last_slash + 1);

        // Names of the form "%%anon-(.*)-\d+" are treated as anonymous
        // nodes from the user's perspective.  We only display the (.*)
        // portion.
        wxString anon_prefix(wxT("%%anon-"));
        size_t anon_prefix_size(anon_prefix.size());
        size_t anon_suffix_start(result.rfind(wxT("-")));
        if (result.substr(0, anon_prefix_size) == anon_prefix &&
            anon_suffix_start != wxString::npos &&
            anon_prefix_size < anon_suffix_start)
        {
            wxString tmp = result.substr(anon_prefix_size,
                                         anon_suffix_start - anon_prefix_size);
            result = tmp;
            is_anonymous = true;
        }
    }

    // Return our results.
    if (outIsAnonymous)
        *outIsAnonymous = is_anonymous;
    return result;
}


//=========================================================================
//  Parent and child relationships
//=========================================================================

bool Node::IsRealChild(ElementPtr inElem) {
    return (inElem->GetParent().get() == this);
}

bool Node::IsChildForPurposeOfZOrderAndVisibility(ElementPtr inElem) {
    // The only reason we should have an element that is not a real
    // child is if it is an artificial element parented to the current
    // group member.  But that case should be caught by the code in
    // GroupMember that overrides this, so we assert that we are only
    // ever dealing with a real child.
    ASSERT(IsRealChild(inElem));
    return !inElem->HasLegacyZOrderAndVisibility();
}

NodePtr Node::GetParentForPurposeOfZOrderAndVisibility() {
    return GetParent();
}


//=========================================================================
//  Events
//=========================================================================

bool Node::ShouldReceiveEventsWhenGrabbing(NodePtr inNode) {
    if (shared_from_this() == inNode) {
        return true;
    } else if (IsRootNode()) {
        return false;
    } else {
        NodePtr parent(GetParentForPurposeOfZOrderAndVisibility());
        return parent->ShouldReceiveEventsWhenGrabbing(inNode);
    }
}


//=========================================================================
//  Compositing
//=========================================================================

void Node::RecursivelyCompositeInto(CairoContext &inCr,
                                    bool inIsCompositingDragLayer,
                                    bool inAncestorIsInDragLayer)
{
    // If this element isn't visible, don't composite it or any of its
    // child elements.
    if (!IsVisible())
        return;

    // Keep track of whether this node (or any of its ancestors) returned
    // true from IsInDragLayer().
    bool is_in_drag_layer = inAncestorIsInDragLayer || IsInDragLayer();

    // Composite this node if we're in the right layer.
    if (is_in_drag_layer == inIsCompositingDragLayer)
        CompositeInto(inCr);

    // Iterate over our child elements.
    BOOST_FOREACH(ElementPtr elem, mElements) {
        if (IsChildForPurposeOfZOrderAndVisibility(elem))
            elem->RecursivelyCompositeInto(inCr, inIsCompositingDragLayer,
                                           is_in_drag_layer);
    }
}


//=========================================================================
//  Hit-testing
//=========================================================================

NodePtr Node::FindNodeAt(const wxPoint &inPoint, bool inMustWantCursor) {
    // First check our elements (in reverse of our compositing order) to
    // see if any of them are interested.
    BOOST_REVERSE_FOREACH(ElementPtr elem, mElements) {
        if (IsChildForPurposeOfZOrderAndVisibility(elem)) {
            NodePtr found(elem->FindNodeAt(inPoint, inMustWantCursor));
            if (found)
                return found;
        }
    }

    // None of our elements matched, so what about us?
    if (IsVisible() && (WantsCursor() || !inMustWantCursor) &&
        IsPointInNode(inPoint))
        return shared_from_this();

    // Nope, nobody here is interested in inPoint.
    return NodePtr();
}


//=========================================================================
//  Node registration and unregistration
//=========================================================================

void Node::Register() {
    NodePtr as_shared(shared_from_this());
    wxGetApp().GetStageFrame()->GetElementsPane()->RegisterNode(as_shared);
}

void Node::Unregister() {
    NodePtr as_shared(shared_from_this());
    wxGetApp().GetStageFrame()->GetElementsPane()->UnregisterNode(as_shared);
    
}

void Node::RecursivelyReregisterWithElementsPane(ElementsPane *inPane) {
    NodePtr as_shared(shared_from_this());
    inPane->RegisterNode(as_shared);
    BOOST_FOREACH(ElementPtr elem, mElements)
        if (IsRealChild(elem))
            elem->RecursivelyReregisterWithElementsPane(inPane);
}

void Node::RegisterChildElement(ElementPtr inElem) {
    ASSERT(std::find(mElements.begin(), mElements.end(), inElem) ==
           mElements.end());
    mElements.push_back(inElem);
}

void Node::UnregisterChildElement(ElementPtr inElem) {
    ElementList::iterator found =
        std::find(mElements.begin(), mElements.end(), inElem);
    ASSERT(found != mElements.end());
    mElements.erase(found);
}


//=========================================================================
//  Other member functions
//=========================================================================

void Node::OperationNotSupported(const char *inOperationName) {
    std::string op(inOperationName);
    std::string name(mName.mb_str());
    THROW("Cannot " + op + " node: " + name);
}

void Node::RaiseToTop(ElementPtr inElem) {
    // Move inElem to the end of mElements.
    ElementList::iterator found =
        std::find(mElements.begin(), mElements.end(), inElem);
    ASSERT(found != mElements.end());
    mElements.erase(found);
    mElements.push_back(inElem);

    // TODO: If IsRealChild(inElem), then update ElementsPane.
}
