//
// Created by dustyn on 12/28/24.
//
#include "include/device/device.h"

#include <include/data_structures/doubly_linked_list.h>
#include <include/device/display/framebuffer.h>

struct doubly_linked_list system_device_tree;


void init_system_device_tree() {
  doubly_linked_list_init(&system_device_tree);
  struct device_group* device_node_1 = kmalloc(sizeof(struct device_group));
  device_node_1->devices = kmalloc(DEVICE_GROUP_SIZE * sizeof(uintptr_t));
  device_node_1->device_major = DEVICE_MAJOR_RAMDISK;

  doubly_linked_list_insert_head(&system_device_tree,device_node_1);

  struct device_group* device_node_2 = kmalloc(sizeof(struct device_group));
  device_node_2->devices = kmalloc(DEVICE_GROUP_SIZE * sizeof(uintptr_t));
  device_node_2->device_major = DEVICE_MAJOR_FRAMEBUFFER;


  doubly_linked_list_insert_head(&system_device_tree,device_node_2);


  struct device_group* device_node_3 = kmalloc(sizeof(struct device_group));
  device_node_3->devices = kmalloc(DEVICE_GROUP_SIZE * sizeof(uintptr_t));
  device_node_3->device_major = DEVICE_MAJOR_SSD;


  doubly_linked_list_insert_head(&system_device_tree,device_node_3);


  struct device_group* device_node_4 = kmalloc(sizeof(struct device_group));
  device_node_4->devices = kmalloc(DEVICE_GROUP_SIZE * sizeof(uintptr_t));
  device_node_4->device_major = DEVICE_MAJOR_HARD_DISK;


  doubly_linked_list_insert_head(&system_device_tree,device_node_4);

  kprintf("System device tree created\n");
}


void insert_device_into_kernel_tree(struct device *device) {

}