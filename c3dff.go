package main

import (
	cpkg "github.com/jurgen-kluft/c3dff/package"
	ccode "github.com/jurgen-kluft/ccode"
)

func main() {
	ccode.Generate(cpkg.GetPackage())
}
