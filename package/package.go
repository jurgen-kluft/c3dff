package c3dff

import (
	cbase "github.com/jurgen-kluft/cbase/package"
	"github.com/jurgen-kluft/ccode/denv"
	ccore "github.com/jurgen-kluft/ccore/package"
	cunittest "github.com/jurgen-kluft/cunittest/package"
)

const (
	repo_path = "github.com\\jurgen-kluft\\"
	repo_name = "c3dff"
)

func GetPackage() *denv.Package {
	name := repo_name

	// Dependencies
	basepkg := cbase.GetPackage()
	corepkg := ccore.GetPackage()
	unittestpkg := cunittest.GetPackage()

	// The main (c3dff) package
	mainpkg := denv.NewPackage(repo_path, repo_name)
	mainpkg.AddPackage(basepkg)
	mainpkg.AddPackage(corepkg)

	// library
	mainlib := denv.SetupCppLibProject(mainpkg, name)
	mainlib.AddDependencies(basepkg.GetMainLib()...)
	mainlib.AddDependencies(corepkg.GetMainLib()...)

	// test library
	testlib := denv.SetupCppTestLibProject(mainpkg, name)
	testlib.AddDependencies(basepkg.GetTestLib()...)
	testlib.AddDependencies(corepkg.GetTestLib()...)

	// unittest project
	maintest := denv.SetupCppTestProject(mainpkg, name)
	maintest.AddDependencies(unittestpkg.GetMainLib()...)
	maintest.AddDependency(testlib)

	mainpkg.AddMainLib(mainlib)
	mainpkg.AddTestLib(testlib)
	mainpkg.AddUnittest(maintest)
	return mainpkg
}
