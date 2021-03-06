// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit Verilog from tree
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2004-2021 by Wilson Snyder. This program is free software; you
// can redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#include "config_build.h"
#include "verilatedos.h"

#include "V3EmitCBase.h"
#include "V3EmitXml.h"
#include "V3Global.h"
#include "V3String.h"

#include <map>
#include <vector>

//######################################################################
// Emit statements and math operators

class EmitXmlFileVisitor final : public AstNVisitor {
  // NODE STATE
  // Entire netlist:
  // AstNode::user1           -> uint64_t, number to connect crossrefs

  // MEMBERS
  V3OutFile *m_ofp;
  uint64_t m_id = 0;

  // METHODS
  VL_DEBUG_FUNC; // Declare debug()

  // Outfile methods
  V3OutFile *ofp() const { return m_ofp; }
  virtual void puts(const string &str) { ofp()->puts(str); }
  virtual void putsNoTracking(const string &str) { ofp()->putsNoTracking(str); }
  virtual void putsQuoted(const string &str) {
    putsNoTracking("\"");
    putsNoTracking(
        V3OutFormatter::quoteNameControls(str, V3OutFormatter::LA_XML));
    putsNoTracking("\"");
  }

  // XML methods
  void outputId(AstNode *nodep) {
    if (!nodep->user1())
      nodep->user1(++m_id);
    puts("\"" + cvtToStr(nodep->user1()) + "\"");
  }
  void outputTag(AstNode *nodep, const string &tagin) {
    string tag = tagin;
    if (tag == "")
      tag = VString::downcase(nodep->typeName());
    puts("<" + tag);
    if (VN_IS(nodep, NodeDType)) {
      puts(" id=");
      outputId(nodep);
    }
    if (nodep->name() != "") {
      puts(" name=");
      putsQuoted(nodep->prettyName());
    }
    if (nodep->tag() != "") {
      puts(" tag=");
      putsQuoted(nodep->tag());
    }
    if (nodep->dtypep()) {
      puts(" dtype_id=");
      outputId(nodep->dtypep()->skipRefp());
    }
  }
  void outputChildrenEnd(AstNode *nodep, const string &tagin) {
    string tag = tagin;
    if (tag == "")
      tag = VString::downcase(nodep->typeName());
    if (nodep->op1p() || nodep->op2p() || nodep->op3p() || nodep->op4p()) {
      puts(">\n");
      iterateChildren(nodep);
      puts("</" + tag + ">\n");
    } else {
      puts("/>\n");
    }
  }

  // VISITORS
  virtual void visit(AstAssignW *nodep) override {
    outputTag(nodep, "contassign"); // IEEE: vpiContAssign
    outputChildrenEnd(nodep, "contassign");
  }
  virtual void visit(AstCell *nodep) override {
    outputTag(nodep, "instance"); // IEEE: vpiInstance
    puts(" defName=");
    putsQuoted(nodep->modName()); // IEEE vpiDefName
    puts(" origName=");
    putsQuoted(nodep->origName());
    outputChildrenEnd(nodep, "instance");
  }
  /**
   * @brief  ?????? Ast ??????
   * @note   ??? RTL ??????
   */
  virtual void visit(AstNetlist *nodep) override {
    puts("<netlist>\n");
    iterateChildren(nodep);
    puts("</netlist>\n");
  }
  /**
   * @brief   ?????? Ast ??????
   * @note    1 - ??? RTL ??????
   *          2 - ?????????????????????????????????????????? Ast ???????????????????????????
   */
  virtual void visit(AstNodeModule *nodep) override {
    outputTag(nodep, "");
    puts(" origName=");
    putsQuoted(nodep->origName());
    if (nodep->level() == 1 ||
        nodep->level() ==
            2) // ==2 because we don't add wrapper when in XML mode
      puts(" topModule=\"1\""); // IEEE vpiTopModule
    if (nodep->modPublic())
      puts(" public=\"true\"");
    outputChildrenEnd(nodep, "");
  }
  /**
   * @brief   ?????? Ast ??? Var
   * @note    1 - ??? RTL ??????
   *          2 - ?????????????????????????????????????????? Ast ???????????????????????????
   *          3 - ?????????????????????var ??????????????????????????????????????? RTL ?????????
   * input???output ?????? wire ???????????? var
   */
  virtual void visit(AstVar *nodep) override {
    const AstVarType typ = nodep->varType();
    const string kw = nodep->verilogKwd();
    const string vt = nodep->dtypep()->name();
    outputTag(nodep, "");
    // ???????????????????????????????????????
    // Hint : ?????? input???output
    if (nodep->isIO()) {
      puts(" dir=");
      putsQuoted(kw);
      if (nodep->pinNum() != 0)
        puts(" pinIndex=\"" + cvtToStr(nodep->pinNum()) + "\"");
      puts(" vartype=");
      putsQuoted(!vt.empty()               ? vt
                 : typ == AstVarType::PORT ? "port"
                                           : "unknown");
      // Hint : ?????? wire
    } else {
      puts(" vartype=");
      putsQuoted(!vt.empty() ? vt : kw);
    }
    puts(" origName=");
    putsQuoted(nodep->origName());
    outputChildrenEnd(nodep, "");
  }

  /**
   * @brief ?????? Ast Pin
   * @note  1 - Pin ??????????????? instance ???????????? pin ???????????????????????????
   *            ?????????????????????????????????????????????????????????????????????
   *        2 - ?????????????????????????????????????????? Ast ???????????????????????????
   */
  virtual void visit(AstPin *nodep) override {
    // What we call a pin in verilator is a port in the IEEE spec.
    outputTag(nodep, "port"); // IEEE: vpiPort
    if (nodep->modVarp()->isIO()) {
      puts(" direction=\"" + nodep->modVarp()->direction().xmlKwd() + "\"");
    }
    // Children includes vpiHighConn and vpiLowConn; we don't support port bits
    // (yet?)
    outputChildrenEnd(nodep, "port");
  }
  /**
   * @brief ????????????????????? visit ??????
   * @note  ????????????????????????????????????????????????
   */
  virtual void visit(AstNode *nodep) override {
    outputTag(nodep, "");
    outputChildrenEnd(nodep, "");
  }

  virtual void visit(AstSel *nodep) override {
    std::string tag = "sel";
    puts("<" + tag);
    outputChildrenEnd(nodep, "");
  }

  virtual void visit(AstVarRef *nodep) override {
    std::string tag = "varref";
    puts("<" + tag);
    puts(" name=");
    putsQuoted(nodep->prettyName());
    std::cout << nodep->prettyName() << std::endl;
    outputChildrenEnd(nodep, "");
  }

  virtual void visit(AstConcat *nodep) override {
    std::string tag = "concat";
    puts("<" + tag);
    outputChildrenEnd(nodep, "");
  }

  virtual void visit(AstConst *nodep) override {
    std::string tag = "const";
    puts("<" + tag);
    puts(" name=");
    std::cout << "\t " << nodep->num() << " : " << nodep->prettyName()
              << std::endl;
    putsQuoted(nodep->prettyName());
    outputChildrenEnd(nodep, "");
  }

public:
  EmitXmlFileVisitor(AstNode *nodep, V3OutFile *ofp) : m_ofp{ofp} {
    iterate(nodep);
  }
  virtual ~EmitXmlFileVisitor() override = default;
};

//######################################################################
// EmitXml class functions

void V3EmitXml::emitxml() {
  const string filename =
      (v3Global.opt.xmlOutput().empty()
           ? v3Global.opt.makeDir() + "/" + v3Global.opt.prefix() + ".xml"
           : v3Global.opt.xmlOutput());
  V3OutXmlFile of(filename);
  EmitXmlFileVisitor visitor{v3Global.rootp(), &of};
}
