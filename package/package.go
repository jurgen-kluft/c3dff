package c3dff

import (
	cbase "github.com/jurgen-kluft/cbase/package"
	"github.com/jurgen-kluft/ccode/denv"
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
	unittestpkg := cunittest.GetPackage()

	// The main (c3dff) package
	mainpkg := denv.NewPackage(name)
	mainpkg.AddPackage(basepkg)

	// library
	mainlib := denv.SetupDefaultCppLibProject(name, repo_path+name)
	mainlib.Dependencies = append(mainlib.Dependencies, basepkg.GetMainLib())

	// unittest project
	maintest := denv.SetupDefaultCppTestProject(name+"_test", repo_path+name)
	maintest.Dependencies = append(maintest.Dependencies, unittestpkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, mainlib)

	mainpkg.AddMainLib(mainlib)
	mainpkg.AddUnittest(maintest)
	return mainpkg
}
