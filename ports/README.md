# ports

vcpkg overlay ports: packages the tree builds from its own port rather than vcpkg's. Both
`vcpkg-configuration.json` files name this directory in `overlay-ports`, and so does a consumer
repository that uses the api.

[docs/contributing/Dependencies.md](../docs/contributing/Dependencies.md) says why each port is
here and how to update it.
