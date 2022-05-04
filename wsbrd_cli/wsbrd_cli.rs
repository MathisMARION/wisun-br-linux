/* SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2021-2022, Silicon Labs
 * Main authors:
 *     - Jérôme Pouiller <jerome.pouiller@silabs.com>
 */
mod wsbrddbusapi;

use std::time::Duration;
use dbus::blocking::Connection;
use dbus::arg::PropMap;
use dbus::arg::prop_cast;
use wsbrddbusapi::ComSilabsWisunBorderRouter;

fn format_byte_array(input: &Vec<u8>) -> String {
    input.iter().map(|n| format!("{:02x}", n)).collect::<Vec<String>>().join(":")
}

fn is_parent(node: &(Vec<u8>, PropMap), target: &Vec<u8>) -> bool {
    let parent: Option<&Vec<u8>> = prop_cast(&node.1, "parent");
    match parent {
        Some(x) if x == target => true,
        Some(_) => false,
        None => false,
    }
}

fn print_rpl_tree(links: &Vec<(Vec<u8>, PropMap)>, parents: &Vec<&Vec<u8>>, cur: &Vec<u8>, indent: &str) -> () {
    let mut children: Vec<&Vec<u8>> = links.iter().filter(|n| is_parent(n, cur)).map(|n| &n.0).collect();
    children.sort();
    let mut new_parents = parents.clone();
    new_parents.push(cur);
    if let Some((last_child, first_childs)) = children.split_last() {
        for c in first_childs {
            if new_parents.contains(c) {
                println!("{}|- {} (loop!)", indent, format_byte_array(c));
            } else {
                println!("{}|- {}", indent, format_byte_array(c));
                print_rpl_tree(links, &new_parents, c, &(indent.to_owned() + "|    "));
            }
        }
        if new_parents.contains(last_child) {
            println!("{}`- {} (loop!)", indent, format_byte_array(last_child));
        } else {
            println!("{}`- {}", indent, format_byte_array(last_child));
            print_rpl_tree(links, &new_parents, last_child, &(indent.to_owned() + "     "));
        }
    }
}

fn do_status(dbus_user: bool) -> Result<(), Box<dyn std::error::Error>> {
    let dbus_conn;
    if dbus_user {
        dbus_conn = Connection::new_session()?;
    } else {
        dbus_conn = Connection::new_system()?;
    }
    let dbus_proxy = dbus_conn.with_proxy("com.silabs.Wisun.BorderRouter", "/com/silabs/Wisun/BorderRouter", Duration::from_millis(500));

    // Consider that if NetworkName does not exist, the service probably not here.
    match dbus_proxy.wisun_network_name() {
        Ok(val) => println!("network_name: {}", val),
        Err(e) => return Err(Box::new(e)),
    }

    println!("domain: {}", dbus_proxy.wisun_domain().unwrap_or("[UNKNOWN]".to_string()));
    println!("mode: {:x}", dbus_proxy.wisun_mode().unwrap_or(0));
    println!("class: {}", dbus_proxy.wisun_class().unwrap_or(0));
    println!("panid: {:#04x}", dbus_proxy.wisun_pan_id().unwrap_or(0));
    println!("size: {}", dbus_proxy.wisun_size().unwrap_or("[UNKNOWN]".to_string()));

    let gaks = dbus_proxy.gaks().unwrap_or(vec![]);
    for (i, g) in gaks.iter().enumerate() {
        println!("GAK[{}]: {}", i, format_byte_array(g));
    }

    let gtks = dbus_proxy.gtks().unwrap_or(vec![]);
    for (i, g) in gtks.iter().enumerate() {
        println!("GTK[{}]: {}", i, format_byte_array(g));
    }

    let mac_br = dbus_proxy.hw_address().unwrap_or(vec![0; 8]);
    println!("{}", format_byte_array(&mac_br));

    let links = dbus_proxy.nodes().unwrap_or(vec![]);
    print_rpl_tree(&links, &vec![], &mac_br, "  ");
    Ok(())
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    do_status(true)
}
