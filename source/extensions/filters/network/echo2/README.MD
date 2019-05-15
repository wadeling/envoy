# 说明
配置filter的顺序: 先配置echo2filter,完成http封包,再配置http_connection_manager来进行路由配置

# 路由配置
```
routes:
- match:
    prefix: "/"
    headers:
    - name: x-test
        exact_match: haha
        route: { cluster: servicetest }
- match:
    prefix: "/"
    headers:
    - name: x-test
        exact_match: ttt
        route: { cluster: servicetest2 }

```
以上配置根据header的不同值来路由

# 后端服务配置
```
clusters:
- name: servicetest
  connect_timeout: 0.25s
  type: LOGICAL_DNS
  dns_lookup_family: V4_ONLY
  lb_policy: ROUND_ROBIN
  load_assignment:
  cluster_name: servicetest
    endpoints:
    - lb_endpoints:
      - endpoint:
          address:
            socket_address:
              address: 127.0.0.1
              port_value: 1090
- name: servicetest2
...

```
以上配置不同的后端服务
