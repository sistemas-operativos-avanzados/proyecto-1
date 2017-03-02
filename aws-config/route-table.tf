# ================================
# Public Traffic Route Table
# ================================

resource "aws_route_table" "soa-public-traffic" {

  vpc_id = "${aws_vpc.soa-vpc.id}"
  route {
    cidr_block = "0.0.0.0/0"
    gateway_id = "${aws_internet_gateway.soa-ig.id}"
  }
  tags {
    Name = "soa-public-traffic"
  }
}

resource "aws_route_table_association" "soa-rta-public-a" {
  subnet_id = "${aws_subnet.soa-public-a.id}"
  route_table_id = "${aws_route_table.soa-public-traffic.id}"
}