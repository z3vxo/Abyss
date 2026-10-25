package main

import (
	"log"

	"github.com/spf13/pflag"
	"github.com/z3vxo/Abyss/client/internal/cli"
)

func main() {
	configStr := pflag.StringP("profile", "p", "", "profile to use")
	debug := pflag.BoolP("debug", "d", false, "debug mode")

	pflag.Parse()
	if *configStr == "" {
		log.Fatal("[!] Profile must be provided")
	}

	app, err := cli.New(*configStr, *debug)
	if err != nil {
		log.Fatal(err)
	}

	if err := app.Run(); err != nil {
		log.Fatal(err)
	}
}
