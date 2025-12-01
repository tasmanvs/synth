#include "audio/nodes/audio_node.h"

namespace audio_nodes {

AudioNode::AudioNode(int node_id, NodeType type)
    : node_id_(node_id)
    , output_pin_id_(node_id * 100 + 1)  // Simple pin ID scheme
    , type_(type) {
}

} // namespace audio_nodes
