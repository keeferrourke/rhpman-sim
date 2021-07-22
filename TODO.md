# TODO

  - Establish original data items and their owner nodes.
    Default should be that 10% of nodes are owners of original data items.
  - Figure out how to query number of nodes within $h$ and $h_r$ hops
  - Figure out to how to query number of direct neighbor nodes.
  - Determine how nodes should decide which data they want.

  - determine the correct TTL that should be used for different message types
    - what should be limited to h_r hops and what is limited to h hops

- what happens to the storage space when a replica holder node steps down form being a replica holder?
  - for this experiment assume that everything in the local storage gets deleted that is not owned by this node when it steps down
- what happens when it steps back up?
- should intermediate nodes that are not replica holders check their local cache? Im assuming that they should
