resource "aws_security_group" "soa-ssh" {
  name        = "soa-ssh"
  description = "SSH access"
  vpc_id = "${aws_vpc.soa-vpc.id}"
  tags {
    Name = "soa-ssh"
  }
}

resource "aws_security_group" "soa-web-server" {
  name = "soa-web-server"
  description = "Web application security"
  vpc_id = "${aws_vpc.soa-vpc.id}"
  tags = {
    Name = "soa-web-server"
  }
}

# ===============================
# soa-ssh rules
# ===============================

resource "aws_security_group_rule" "soa-ssh-in" {
  type              = "ingress"
  from_port         = 22
  to_port           = 22
  protocol          = "tcp"
  cidr_blocks       = ["0.0.0.0/0"]
  security_group_id = "${aws_security_group.soa-ssh.id}"
}

resource "aws_security_group_rule" "soa-ssh-out" {
  type = "egress"
  from_port = 0
  to_port = 0
  protocol = "-1"
  cidr_blocks = ["0.0.0.0/0"]
  security_group_id = "${aws_security_group.soa-ssh.id}"
}

# ===============================
# soa-web-server rules
# ===============================

resource "aws_security_group_rule" "web-server-http-in" {
  type              = "ingress"
  from_port         = 80
  to_port           = 80
  protocol          = "tcp"
  cidr_blocks       = ["0.0.0.0/0"]
  security_group_id = "${aws_security_group.soa-web-server.id}"
}

resource "aws_security_group_rule" "soa-web-server-http-in" {
  type              = "ingress"
  from_port         = 8080
  to_port           = 8084
  protocol          = "tcp"
  cidr_blocks       = ["0.0.0.0/0"]
  security_group_id = "${aws_security_group.soa-web-server.id}"
}

resource "aws_security_group_rule" "web-server-http-out" {
  type              = "egress"
  from_port         = 80
  to_port           = 80
  protocol          = "tcp"
  cidr_blocks       = ["0.0.0.0/0"]
  security_group_id = "${aws_security_group.soa-web-server.id}"
} 


resource "aws_security_group_rule" "soa-web-server-http-out" {
  type              = "egress"
  from_port         = 8080
  to_port           = 8084
  protocol          = "tcp"
  cidr_blocks       = ["0.0.0.0/0"]
  security_group_id = "${aws_security_group.soa-web-server.id}"
}  