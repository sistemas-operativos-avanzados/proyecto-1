output "vpc-id" {
  value = "${aws_vpc.soa-vpc.id}"
}

output "public-subnets" {
  value = "${aws_subnet.soa-public-a.id}"
}

output "soa-web-server-sg" {
  value = "${aws_security_group.soa-web-server.id}"
}

output "soa-ssh-sg" {
  value = "${aws_security_group.soa-ssh.id}"
}

output "bastion-id" {
  value = "${aws_instance.soa-bastion.id}"
}

output "bastion-public-dns" {
  value = "${aws_instance.soa-bastion.public_dns}"
}

output "bastion-public-ip" {
  value = "${aws_instance.soa-bastion.public_ip}"
}

output "bastion-private-ip" {
  value = "${aws_instance.soa-bastion.private_ip}"
}