#include "./graph/base.h"
#include <iostream>
#include <string>

using namespace htps;

int main() {
//    std::string json = R"(
//    {
//        "proof_theorem": {
//            "conclusion": "a",
//            "hypotheses": [
//                {
//                    "identifier": "b",
//                    "type": "c"
//                }
//            ],
//            "unique_string": "d"
//        },
//        "proof_tactic": {
//            "unique_string": "e",
//            "is_valid": true,
//            "duration": 200
//        },
//        "children": [
//            {
//                "conclusion": "f",
//                "hypotheses": [
//                    {
//                        "identifier": "g",
//                        "type": "h"
//                    }
//                ],
//                "unique_string": "i"
//            }
//        ]
//    }
//    )";
//    proof_node node = proof_node::from_json(json);

//    // Try hyper tree node
//    json = R"(
//    {
//        "id": 0,
//        "_proven": false,
//        "tactics": [
//            {
//                "unique_string": "a",
//                "is_valid": true,
//                "duration": 200
//            }
//        ],
//        "tactic_to_children": {
//            "a": [
//                {
//                    "id": 1,
//                    "_proven": false,
//                    "tactics": [
//                        {
//                            "unique_string": "b",
//                            "is_valid": true,
//                            "duration": 400
//                        }
//                    ],
//                    "tactic_to_children": {
//                        "b": []
//                    }
//                }
//            ]
//        }
//    }
//    )";
//    hyper_tree_node htn = hyper_tree_node::from_json(json);
//
//    hyper_tree ht;
//    ht.add_node(node.proof_theorem);
//    ht.expand_with_tactic(node.proof_theorem, node.proof_tactic, node.children);
////    ht.propagate_proven(node.proof_theorem);
//    proof p = ht.get_proof();
//    std::cout << p.proof_theorem.conclusion << std::endl;
    return 0;
}

