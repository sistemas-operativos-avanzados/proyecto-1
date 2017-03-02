variable "aws_access_key" { }
variable "aws_secret_key" { }
variable "aws_key_name" { }

variable "aws_region" {
  description = "The AWS region to build this in"
}

variable "vpc_cidr_block" {
  description = "VPC CIDR block"
}

variable "subnet_public_a_cidr" {
  description = "Subnet Public A CIDR"
}

variable "subnet_public_a_region" {
  description = "Subnet Public A Region"
}
