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
	mainpkg := denv.NewPackage(name)
	mainpkg.AddPackage(basepkg)
	mainpkg.AddPackage(corepkg)

	// library
	mainlib := denv.SetupCppLibProject(name, repo_path+name)
	mainlib.AddDependencies(basepkg.GetMainLib()...)
	mainlib.AddDependencies(corepkg.GetMainLib()...)

	// unittest project
	maintest := denv.SetupDefaultCppTestProject(name+"_test", repo_path+name)
	maintest.AddDependencies(basepkg.GetMainLib()...)
	maintest.AddDependencies(corepkg.GetMainLib()...)
	maintest.AddDependencies(unittestpkg.GetMainLib()...)
	maintest.Dependencies = append(maintest.Dependencies, mainlib)

	mainpkg.AddMainLib(mainlib)
	mainpkg.AddUnittest(maintest)
	return mainpkg
}
