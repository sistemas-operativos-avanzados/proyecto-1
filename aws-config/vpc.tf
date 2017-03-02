provider "aws"{
  access_key  = "${var.aws_access_key}"
  secret_key  = "${var.aws_secret_key}"
  region      = "${var.aws_region}"
}

# The VPC where resources will live
resource "aws_vpc" "soa-vpc"  {
  cidr_block = "${var.vpc_cidr_block}"
  enable_dns_hostnames = true
  tags {
    Name = "soa vpc"
  }
}

# ================================
# Internet gateway
# ================================
resource "aws_internet_gateway" "soa-ig" {
  vpc_id = "${aws_vpc.soa-vpc.id}"

  tags {
    Name = "soa ig"
  }
}

# ================================
# Bastion Host
# ================================
resource "aws_instance" "soa-bastion" {
  ami                         = "ami-49c9295f" # Ubuntu 14.04 LTS
  availability_zone           = "us-east-1a"
  instance_type               = "t2.medium"
  key_name                    = "${var.aws_key_name}"
  vpc_security_group_ids      = ["${aws_security_group.soa-ssh.id}", "${aws_security_group.soa-web-server.id}"]
  subnet_id                   = "${aws_subnet.soa-public-a.id}"
  associate_public_ip_address = true
  source_dest_check           = true
  provisioner "remote-exec" {
    script = "bootstrap.sh"
    connection {
      type = "ssh"
      user = "ubuntu"
      private_key = "${file("${path.module}/mfadmin-aws-auth.pem")}"
    }   
  }

  tags {
    Name = "soa-bastion"
  }
}