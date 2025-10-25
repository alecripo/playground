import operator


# The TCP header is the 1s complement of the sum of 1s complement of:
# - the pseudo tcp header, composed of the following information for IPv4:
#   - source ip
#   - dest ip
#   - next byte set to 0
#   - IP Protocol number
#   - TCP length: tcp header + tcp data
# - the TCP header, with 0s for the checksum
# - the TCP data, padded with 0s to the right if it is not a multiple of 16

class InvalidAddress(Exception):
    def __init__(self, addr: str):
        self.add_note(f"Invalid address {addr}")


def ip_int2str(addr: int) -> str:
    addr_str = ""
    mask = 0xFF << 24
    shift = 24
    for _ in range(4):
        addr_str += str((addr & mask) >> shift) + "."
        shift -= 8
        mask = mask >> 8
    return addr_str[:-1]


def ip_str2int(addr: str) -> int:
    parts = addr.split(".")
    if len(parts) != 4:
        raise InvalidAddress(addr)
    addr_int = 0
    shift_modifier = 0
    for part in parts:
        addr_int += int(part) << (24 - shift_modifier)
        shift_modifier += 8
    return addr_int


def tcp_pseudo_hdr(src_ip: str, dst_ip: str, tcp_length: int) -> bytearray:
    hdr = bytearray()

    hdr += ip_str2int(src_ip).to_bytes(4)
    hdr += ip_str2int(dst_ip).to_bytes(4)
    hdr += int(0).to_bytes()
    hdr += int(0x06).to_bytes()
    hdr += tcp_length.to_bytes(2)

    assert len(hdr) * 8 == 96, f"Invalid header length: expected 94, got {len(hdr) * 8}"
    return hdr


def checksum(ip_hdr: bytearray, tcp_segment: bytearray) -> int:
    tcp_segment = tcp_segment[:16] + b"\x00\x00" + tcp_segment[18:]
    if len(tcp_segment) % 2 == 1:
        tcp_segment += b"\x00"
    chksum = 0
    data = ip_hdr + tcp_segment
    for i in range(0, len(data) - 1, 2):
        part = int.from_bytes(data[i : i + 2], "big")
        chksum += part
        chksum = (chksum & 0xFFFF) + (chksum >> 16)
    return operator.xor(chksum, 0xFFFF)


def main():
    num_pairs = 10
    for i in range(num_pairs):
        with open(f"tcp_addrs_{i}.txt", "r") as f, open(f"tcp_data_{i}.dat", "rb") as g:
            addresses = f.readline().split(" ")
            if len(addresses) != 2:
                continue
            src = addresses[0].strip()
            dst = addresses[1].strip()
            packet = bytearray(g.read())
            actual_checksum = int.from_bytes(packet[16:18], "big")

            tcp_pseudo_header = tcp_pseudo_hdr(src, dst, len(packet))
            comp_chksum = checksum(tcp_pseudo_header, packet.copy())

            if comp_chksum != actual_checksum:
                print(f"File {i+1}: FAIL")
                print(f"computed: {comp_chksum:016b}")
                print(f"actual:   {actual_checksum:016b}")
            else:
                print(f"File {i+1}: PASS")


if __name__ == "__main__":
    main()
