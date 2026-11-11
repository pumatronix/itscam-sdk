// Lists the exported API of a Go package, one declaration per line.
//
// Copyright (c) 2026 Pumatronix
//
//	go run ./tools/compat/api_go <path to src/wrappers/go/itscam>
//
// Parses the sources only (no cgo build): exported functions and methods
// with their parameter and result types, exported struct fields with types
// and tags, interface methods, other types, constants and variables.
// Parameter names are left out: renaming one doesn't break a caller.
package main

import (
	"bytes"
	"fmt"
	"go/ast"
	"go/parser"
	"go/printer"
	"go/token"
	"os"
	"sort"
	"strings"
)

var fset = token.NewFileSet()

func expr(e ast.Expr) string {
	var b bytes.Buffer
	printer.Fprint(&b, fset, e)
	return strings.Join(strings.Fields(b.String()), " ")
}

func fieldTypes(list *ast.FieldList) string {
	if list == nil {
		return ""
	}
	var parts []string
	for _, f := range list.List {
		n := len(f.Names)
		if n == 0 {
			n = 1
		}
		for i := 0; i < n; i++ {
			parts = append(parts, expr(f.Type))
		}
	}
	return strings.Join(parts, ", ")
}

func funcType(t *ast.FuncType) string {
	s := "(" + fieldTypes(t.Params) + ")"
	if r := fieldTypes(t.Results); r != "" {
		s += " (" + r + ")"
	}
	return s
}

func receiverType(r *ast.FieldList) (string, bool) {
	t := r.List[0].Type
	if s, ok := t.(*ast.StarExpr); ok {
		t = s.X
	}
	if ix, ok := t.(*ast.IndexExpr); ok {
		t = ix.X
	}
	id, ok := t.(*ast.Ident)
	if !ok {
		return "", false
	}
	return expr(r.List[0].Type), id.IsExported()
}

func main() {
	pkgs, err := parser.ParseDir(fset, os.Args[1], func(fi os.FileInfo) bool {
		return !strings.HasSuffix(fi.Name(), "_test.go")
	}, 0)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	var lines []string
	add := func(format string, args ...interface{}) { lines = append(lines, fmt.Sprintf(format, args...)) }
	for _, pkg := range pkgs {
		for path, file := range pkg.Files {
			// version.go is generated per build (tools/version/gen-version.sh):
			// its constants are API, their values (dates, commits) are not
			generated := strings.HasSuffix(path, "/version.go")
			for _, decl := range file.Decls {
				switch d := decl.(type) {
				case *ast.FuncDecl:
					if !d.Name.IsExported() {
						continue
					}
					if d.Recv != nil {
						recv, exported := receiverType(d.Recv)
						if !exported {
							continue
						}
						add("func (%s) %s%s", recv, d.Name.Name, funcType(d.Type))
					} else {
						add("func %s%s", d.Name.Name, funcType(d.Type))
					}
				case *ast.GenDecl:
					for _, spec := range d.Specs {
						switch s := spec.(type) {
						case *ast.TypeSpec:
							if !s.Name.IsExported() {
								continue
							}
							switch t := s.Type.(type) {
							case *ast.StructType:
								add("type %s struct", s.Name.Name)
								for _, f := range t.Fields.List {
									tag := ""
									if f.Tag != nil {
										tag = " " + f.Tag.Value
									}
									if len(f.Names) == 0 {
										add("type %s struct: embeds %s%s", s.Name.Name, expr(f.Type), tag)
									}
									for _, n := range f.Names {
										if n.IsExported() {
											add("type %s struct: %s %s%s", s.Name.Name, n.Name, expr(f.Type), tag)
										}
									}
								}
							case *ast.InterfaceType:
								add("type %s interface", s.Name.Name)
								for _, m := range t.Methods.List {
									for _, n := range m.Names {
										if ft, ok := m.Type.(*ast.FuncType); ok {
											add("type %s interface: %s%s", s.Name.Name, n.Name, funcType(ft))
										}
									}
									if len(m.Names) == 0 {
										add("type %s interface: embeds %s", s.Name.Name, expr(m.Type))
									}
								}
							default:
								assign := " "
								if s.Assign.IsValid() {
									assign = " = "
								}
								add("type %s%s%s", s.Name.Name, assign, expr(s.Type))
							}
						case *ast.ValueSpec:
							kind := "var"
							if d.Tok == token.CONST {
								kind = "const"
							}
							for i, n := range s.Names {
								if !n.IsExported() {
									continue
								}
								line := kind + " " + n.Name
								if s.Type != nil {
									line += " " + expr(s.Type)
								}
								if kind == "const" && i < len(s.Values) && !generated {
									line += " = " + expr(s.Values[i])
								}
								add("%s", line)
							}
						}
					}
				}
			}
		}
	}
	sort.Strings(lines)
	prev := ""
	for _, l := range lines {
		if l != prev {
			fmt.Println(l)
		}
		prev = l
	}
}
