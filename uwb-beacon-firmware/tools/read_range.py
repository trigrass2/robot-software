#!/usr/bin/env python3
"""
Reads range information sent by the beacon over UAVCAN.
"""

import argparse
import uavcan
import os
import csv


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", help="Serial port or SocketCAN interface")
    parser.add_argument(
        "--output",
        "-o",
        help="Log range messages to a file in CSV",
        type=argparse.FileType("w"),
    )

    parser.add_argument(
        "--anchor",
        "-a",
        help="Accept only ranging coming from the given anchor ID",
        type=int,
    )

    return parser.parse_args()


def range_cb(event):
    msg = event.message
    print(msg)


def main():
    args = parse_args()
    if args.output:
        output_file = csv.DictWriter(args.output, ["ts", "anchor_addr", "range"])
        output_file.writeheader()

    def range_cb(event):
        msg = event.message

        if args.anchor and msg.anchor_addr != args.anchor:
            return

        print("Received a range from {}: {:.3f}".format(msg.anchor_addr, msg.range))
        if args.output:
            output_file.writerow(
                {
                    "ts": msg.timestamp.usec,
                    "range": msg.range,
                    "anchor_addr": msg.anchor_addr,
                }
            )

            args.output.flush()

    node = uavcan.make_node(args.port, node_id=127)

    # TODO This path is a bit too hardcoded
    dsdl_path = os.path.join(
        os.path.dirname(__file__), "..", "..", "uavcan_data_types", "cvra", "uwb_beacon"
    )
    uavcan.load_dsdl(dsdl_path)

    node.add_handler(uavcan.thirdparty.uwb_beacon.RadioRange, range_cb)

    node.spin()


if __name__ == "__main__":
    main()
