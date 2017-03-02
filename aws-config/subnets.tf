# ================================
# Subnets
# ================================

# Subnet soa-public-a
resource "aws_subnet" "soa-public-a" {
  vpc_id                    = "${aws_vpc.soa-vpc.id}"
  cidr_block                = "${var.subnet_public_a_cidr}"
  availability_zone         = "${var.subnet_public_a_region}"
  map_public_ip_on_launch   = true
  tags {
    Name = "soa-public-a"
  }
}