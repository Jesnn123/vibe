---
layout: default
title: VIBE Examples
description: Real-world examples showcasing VIBE configuration language features
permalink: /examples/
---

# VIBE Examples

<div class="examples-intro">
  <p class="lead">
    Explore real-world examples that demonstrate VIBE's clean syntax and powerful features. 
    Each example is designed to show best practices and common patterns.
  </p>
</div>

## Table of Contents

1. [Web Application](#web-application)
2. [Microservices Configuration](#microservices)
3. [Database Setup](#database-setup)
4. [CI/CD Pipeline](#cicd-pipeline)
5. [Game Server Configuration](#game-server)
6. [Monitoring & Logging](#monitoring)
7. [Desktop Application](#desktop-app)
8. [API Gateway](#api-gateway)

---

## Web Application {#web-application}

Complete configuration for a modern web application with authentication, database, and caching.

```vibe
# Web Application Configuration
# Production-ready setup with security and performance optimizations

app {
  name "MyWebApp"
  version "2.1.0"
  environment production
  
  # Server configuration
  server {
    host 0.0.0.0
    port 8080
    workers 8
    max_connections 1000
    
    # SSL/TLS settings
    ssl {
      enabled true
      cert_path "/etc/ssl/certs/webapp.crt"
      key_path "/etc/ssl/private/webapp.key"
      protocols [TLSv1.2 TLSv1.3]
      ciphers "ECDHE+AESGCM:ECDHE+CHACHA20:DHE+AESGCM:DHE+CHACHA20:!aNULL:!MD5:!DSS"
    }
    
    # Security headers
    security {
      csrf_protection true
      xss_protection true
      content_type_nosniff true
      frame_options DENY
      hsts_max_age 31536000
    }
  }
  
  # Database configuration
  database {
    primary {
      type postgresql
      host db-primary.internal
      port 5432
      name webapp_prod
      username webapp_user
      # Password loaded from environment variable
      
      pool {
        min_size 5
        max_size 50
        idle_timeout 300
        max_lifetime 1800
      }
      
      # Connection options
      options {
        sslmode require
        connect_timeout 10
        statement_timeout 30000
        lock_timeout 5000
      }
    }
    
    # Read replica for analytics
    replica {
      type postgresql
      host db-replica.internal
      port 5432
      name webapp_prod
      username webapp_readonly
      
      pool {
        min_size 2
        max_size 20
        idle_timeout 600
      }
    }
  }
  
  # Redis cache configuration
  cache {
    primary {
      host redis-primary.internal
      port 6379
      database 0
      max_connections 100
      
      # Cluster configuration
      cluster {
        enabled true
        nodes [
          "redis-1.internal:6379"
          "redis-2.internal:6379"
          "redis-3.internal:6379"
        ]
      }
    }
    
    # Session storage
    sessions {
      host redis-sessions.internal
      port 6379
      database 1
      ttl 3600
    }
  }
  
  # Authentication & Authorization
  auth {
    jwt {
      secret_key "your-256-bit-secret"
      algorithm HS256
      expires_in 3600
      refresh_expires_in 604800
    }
    
    oauth {
      google {
        client_id "your-google-client-id"
        enabled true
        scopes [email profile]
      }
      
      github {
        client_id "your-github-client-id"
        enabled true
        scopes [user:email]
      }
    }
    
    # Password requirements
    password_policy {
      min_length 8
      require_uppercase true
      require_lowercase true
      require_numbers true
      require_symbols true
      max_age_days 90
    }
  }
  
  # File storage
  storage {
    uploads {
      type s3
      bucket "webapp-uploads-prod"
      region "us-west-2"
      max_file_size "50MB"
      allowed_types [jpg jpeg png gif pdf doc docx]
    }
    
    static {
      type cdn
      base_url "https://cdn.mywebapp.com"
      cache_ttl 86400
    }
  }
  
  # Email configuration
  email {
    provider sendgrid
    from_address "noreply@mywebapp.com"
    from_name "MyWebApp"
    
    templates {
      welcome "welcome-email-template"
      password_reset "password-reset-template"
      notification "notification-template"
    }
    
    # Rate limiting
    rate_limits {
      per_user_hourly 10
      per_user_daily 50
    }
  }
  
  # Monitoring & Analytics
  monitoring {
    metrics {
      enabled true
      endpoint "/metrics"
      collectors [cpu memory database_connections response_times]
    }
    
    health_checks {
      endpoint "/health"
      detailed_endpoint "/health/detailed"
      interval 30
      timeout 10
    }
    
    analytics {
      google_analytics "GA-XXXXXXXXX"
      mixpanel "your-mixpanel-token"
      enable_user_tracking true
    }
  }
}

# Feature flags
features {
  new_dashboard {
    enabled true
    rollout_percentage 100
    user_groups [premium beta_testers]
  }
  
  experimental_search {
    enabled false
    rollout_percentage 5
    user_groups [beta_testers]
  }
  
  social_login {
    enabled true
    providers [google github facebook]
  }
}

# Scheduled tasks
scheduler {
  cleanup_old_sessions {
    cron "0 2 * * *"  # Daily at 2 AM
    enabled true
  }
  
  send_weekly_digest {
    cron "0 9 * * 1"  # Monday at 9 AM
    enabled true
    timezone "UTC"
  }
  
  backup_database {
    cron "0 3 * * *"  # Daily at 3 AM
    retention_days 30
    enabled true
  }
}
```

---

## Microservices Configuration {#microservices}

Configuration for a microservices architecture with service discovery, load balancing, and circuit breakers.

```vibe
# Microservices Platform Configuration
platform {
  name "ECommerce Platform"
  version "3.2.1"
  environment production
  
  # Service registry
  service_registry {
    type consul
    address "consul.internal:8500"
    health_check_interval 30
    deregister_critical_after 300
  }
  
  # Message broker
  message_broker {
    type kafka
    brokers [
      "kafka-1.internal:9092"
      "kafka-2.internal:9092"
      "kafka-3.internal:9092"
    ]
    
    topics {
      user_events {
        partitions 12
        replication_factor 3
        retention_hours 168  # 7 days
      }
      
      order_events {
        partitions 6
        replication_factor 3
        retention_hours 720  # 30 days
      }
      
      payment_events {
        partitions 3
        replication_factor 3
        retention_hours 2160  # 90 days
      }
    }
  }
}

# Individual Services
services {
  # User service
  user_service {
    name "user-service"
    version "2.1.0"
    replicas 4
    
    container {
      image "ecommerce/user-service:2.1.0"
      cpu_limit "500m"
      memory_limit "512Mi"
      ports [8080]
    }
    
    database {
      type postgresql
      host "user-db.internal"
      port 5432
      name "users"
      connection_pool_size 20
    }
    
    cache {
      type redis
      host "user-cache.internal"
      port 6379
      ttl 3600
    }
    
    api {
      base_path "/api/v1/users"
      rate_limit 1000  # requests per minute
      auth_required true
    }
    
    health_check {
      path "/health"
      interval 15
      timeout 5
      failure_threshold 3
    }
  }
  
  # Product catalog service
  product_service {
    name "product-service"
    version "1.8.2"
    replicas 6
    
    container {
      image "ecommerce/product-service:1.8.2"
      cpu_limit "750m"
      memory_limit "1Gi"
      ports [8080]
    }
    
    database {
      type mongodb
      connection_string "mongodb://product-cluster.internal:27017/products"
      max_pool_size 50
    }
    
    search {
      type elasticsearch
      hosts [
        "es-1.internal:9200"
        "es-2.internal:9200"
        "es-3.internal:9200"
      ]
      index "products"
    }
    
    api {
      base_path "/api/v1/products"
      rate_limit 2000
      cache_ttl 300
    }
  }
  
  # Order service
  order_service {
    name "order-service"
    version "2.3.1"
    replicas 5
    
    container {
      image "ecommerce/order-service:2.3.1"
      cpu_limit "1000m"
      memory_limit "1.5Gi"
      ports [8080]
    }
    
    database {
      type postgresql
      host "order-db.internal"
      port 5432
      name "orders"
      connection_pool_size 30
    }
    
    dependencies [user_service product_service payment_service]
    
    circuit_breaker {
      failure_threshold 5
      recovery_timeout 30
      half_open_max_calls 3
    }
    
    api {
      base_path "/api/v1/orders"
      rate_limit 500
      auth_required true
    }
  }
  
  # Payment service
  payment_service {
    name "payment-service"
    version "1.5.0"
    replicas 3
    
    container {
      image "ecommerce/payment-service:1.5.0"
      cpu_limit "500m"
      memory_limit "512Mi"
      ports [8080]
      
      # Security requirements
      security_context {
        run_as_non_root true
        read_only_root_filesystem true
      }
    }
    
    database {
      type postgresql
      host "payment-db.internal"
      port 5432
      name "payments"
      ssl_mode "require"
      connection_pool_size 10
    }
    
    external_apis {
      stripe {
        base_url "https://api.stripe.com"
        timeout 30
        retry_attempts 3
      }
      
      paypal {
        base_url "https://api.paypal.com"
        timeout 45
        retry_attempts 2
      }
    }
    
    security {
      encryption_at_rest true
      audit_logging true
      pci_compliance true
    }
    
    api {
      base_path "/api/v1/payments"
      rate_limit 200
      auth_required true
      encryption_required true
    }
  }
  
  # Notification service
  notification_service {
    name "notification-service"
    version "1.2.3"
    replicas 2
    
    container {
      image "ecommerce/notification-service:1.2.3"
      cpu_limit "250m"
      memory_limit "256Mi"
      ports [8080]
    }
    
    channels {
      email {
        provider "sendgrid"
        templates_path "/templates/email"
        rate_limit 1000  # per hour
      }
      
      sms {
        provider "twilio"
        rate_limit 100  # per hour
      }
      
      push {
        provider "firebase"
        rate_limit 5000  # per hour
      }
    }
    
    queue {
      type redis
      host "notification-queue.internal"
      port 6379
      batch_size 100
      retry_attempts 3
    }
  }
}

# Load balancer configuration
load_balancer {
  type nginx
  algorithm least_connections
  
  upstream_groups {
    user_api {
      servers [
        user-service-1.internal:8080
        user-service-2.internal:8080
        user-service-3.internal:8080
        user-service-4.internal:8080
      ]
      health_check_path "/health"
    }
      
    product_api {
      servers [
        product-service-1.internal:8080
        product-service-2.internal:8080
        product-service-3.internal:8080
        product-service-4.internal:8080
        product-service-5.internal:8080
        product-service-6.internal:8080
      ]
      health_check_path "/health"
    }
  }
  
  ssl {
    certificate "/etc/ssl/platform.crt"
    private_key "/etc/ssl/platform.key"
    protocols [TLSv1.2 TLSv1.3]
  }
  
  rate_limiting {
    global_limit 10000  # requests per minute
    per_ip_limit 100   # requests per minute per IP
  }
}

# Monitoring configuration
monitoring {
  prometheus {
    scrape_interval 15
    retention "30d"
    
    targets {
      services [user_service product_service order_service payment_service notification_service]
      infrastructure [consul kafka redis postgresql mongodb elasticsearch]
    }
  }
  
  alerting {
    slack {
      webhook_url "https://hooks.slack.com/services/..."
      channel "#alerts"
    }
    
    rules {
      high_error_rate {
        condition "error_rate > 5%"
        duration "5m"
        severity critical
      }
      
      high_latency {
        condition "response_time_p95 > 2s"
        duration "3m"
        severity warning
      }
      
      service_down {
        condition "up == 0"
        duration "1m"
        severity critical
      }
    }
  }
  
  logging {
    level info
    format json
    
    aggregation {
      type elasticsearch
      index_pattern "microservices-logs-*"
      retention_days 30
    }
  }
}
```

---

## Database Setup {#database-setup}

Multi-environment database configuration with replication, backup, and monitoring.

```vibe
# Database Configuration
# Multi-environment setup with PostgreSQL primary/replica

environments {
  development {
    database {
      primary {
        type postgresql
        host localhost
        port 5432
        name myapp_dev
        username dev_user
        password "dev_password_123"
        
        # Development-specific settings
        settings {
          log_statement all
          log_min_duration_statement 0
          shared_preload_libraries "pg_stat_statements"
          track_activity_query_size 2048
        }
        
        connection_pool {
          min_connections 2
          max_connections 10
          idle_timeout 300
        }
      }
      
      # Test data seeding
      seed_data {
        enabled true
        files [
          seeds/users.sql
          seeds/products.sql
          seeds/orders.sql
        ]
      }
    }
  }
  
  staging {
    database {
      primary {
        type postgresql
        host staging-db.internal
        port 5432
        name myapp_staging
        username staging_user
        # Password from environment variable
        
        settings {
          shared_buffers "256MB"
          effective_cache_size "1GB"
          work_mem "4MB"
          maintenance_work_mem "64MB"
          checkpoint_completion_target 0.9
          wal_buffers "16MB"
          default_statistics_target 100
        }
        
        connection_pool {
          min_connections 5
          max_connections 25
          idle_timeout 600
        }
        
        ssl {
          enabled true
          mode require
          cert_file "/etc/ssl/postgresql.crt"
          key_file "/etc/ssl/postgresql.key"
        }
      }
      
      # Read replica for testing load
      replica {
        type postgresql
        host staging-replica.internal
        port 5432
        name myapp_staging
        username staging_readonly
        
        replication {
          mode streaming
          lag_threshold "10MB"
          recovery_target_timeline latest
        }
        
        connection_pool {
          min_connections 2
          max_connections 15
        }
      }
    }
  }
  
  production {
    database {
      primary {
        type postgresql
        host prod-primary.internal
        port 5432
        name myapp_prod
        username prod_user
        # Password from secure vault
        
        # Production-optimized settings
        settings {
          shared_buffers "2GB"
          effective_cache_size "6GB"
          work_mem "8MB"
          maintenance_work_mem "512MB"
          checkpoint_completion_target 0.9
          wal_buffers "64MB"
          default_statistics_target 500
          random_page_cost 1.1
          effective_io_concurrency 200
          max_worker_processes 8
          max_parallel_workers_per_gather 4
          max_parallel_workers 8
        }
        
        connection_pool {
          min_connections 20
          max_connections 100
          idle_timeout 900
          max_lifetime 3600
          health_check_period 30
        }
        
        ssl {
          enabled true
          mode require
          cert_file "/etc/ssl/postgresql.crt"
          key_file "/etc/ssl/postgresql.key"
          ca_file "/etc/ssl/ca.crt"
        }
        
        # High availability
        failover {
          enabled true
          check_interval 5
          timeout 30
          auto_failback false
        }
      }
      
      # Read replicas
      replicas {
        replica1 {
          host prod-replica1.internal
          port 5432
          lag_threshold "50MB"
          priority 100
        }
        replica2 {
          host prod-replica2.internal
          port 5432
          lag_threshold "50MB"
          priority 90
        }
        analytics {
          host prod-analytics.internal
          port 5432
          lag_threshold "500MB"  # More relaxed for analytics
          priority 50
        }
      }
      
      # Backup configuration
      backup {
        enabled true
        type "continuous"
        
        # Base backups
        base_backup {
          schedule "0 2 * * *"  # Daily at 2 AM
          retention_days 30
          compression true
          checksum_validation true
          storage {
            type s3
            bucket "myapp-database-backups"
            region "us-west-2"
            encryption true
          }
        }
        
        # WAL archiving
        wal_archive {
          enabled true
          storage {
            type s3
            bucket "myapp-wal-archive"
            region "us-west-2"
            encryption true
          }
          retention_days 30
        }
        
        # Point-in-time recovery
        pitr {
          enabled true
          retention_days 7
        }
      }
      
      # Monitoring
      monitoring {
        enabled true
        
        metrics {
          connection_count true
          query_performance true
          replication_lag true
          disk_usage true
          cache_hit_ratio true
          deadlocks true
          slow_queries true
        }
        
        alerts {
          high_connection_usage {
            threshold 80  # percentage
            duration "5m"
            severity warning
          }
          
          replication_lag {
            threshold "100MB"
            duration "2m"
            severity critical
          }
          
          disk_space_low {
            threshold 85  # percentage
            duration "5m"
            severity warning
          }
          
          slow_query_detected {
            threshold "10s"
            severity warning
          }
        }
      }
      
      # Maintenance
      maintenance {
        vacuum {
          auto_vacuum true
          auto_vacuum_scale_factor 0.1
          auto_vacuum_analyze_scale_factor 0.05
          
          # Manual vacuum schedule
          schedule "0 3 * * 0"  # Weekly on Sunday at 3 AM
        }
        
        analyze {
          auto_analyze true
          schedule "0 4 * * *"  # Daily at 4 AM
        }
        
        reindex {
          schedule "0 5 * * 0"  # Weekly on Sunday at 5 AM
          concurrent true
        }
      }
    }
  }
}

# Migration configuration
migrations {
  directory "migrations/"
  table_name "schema_migrations"
  
  # Migration strategies per environment
  strategies {
    development {
      auto_migrate true
      rollback_on_failure true
    }
    
    staging {
      auto_migrate false
      require_approval true
      dry_run_first true
    }
    
    production {
      auto_migrate false
      require_approval true
      backup_before_migration true
      dry_run_first true
      rollback_timeout 300
    }
  }
}

# Connection pooling
connection_pools {
  pgbouncer {
    enabled true
    port 6432
    
    pools {
      application {
        database myapp_prod
        user prod_user
        pool_mode transaction
        pool_size 25
        reserve_pool_size 5
        max_client_conn 100
      }
      
      readonly {
        database myapp_prod
        user prod_readonly
        pool_mode session
        pool_size 15
        reserve_pool_size 3
        max_client_conn 50
      }
    }
    
    # Authentication
    auth_type md5
    auth_file "/etc/pgbouncer/userlist.txt"
    
    # Logging
    log_connections true
    log_disconnections true
    log_pooler_errors true
  }
}

# Database security
security {
  # Access control
  access_control {
    allowed_hosts [
      "10.0.0.0/8"      # Internal network
      "172.16.0.0/12"   # Container network
      "192.168.0.0/16"  # Private network
    ]
    
    denied_hosts [
      "0.0.0.0/0"  # Block everything else
    ]
  }
  
  # SSL/TLS configuration
  ssl {
    cert_file "/etc/ssl/certs/postgresql.crt"
    key_file "/etc/ssl/private/postgresql.key"
    ca_file "/etc/ssl/certs/ca.crt"
    crl_file "/etc/ssl/certs/postgresql.crl"
    
    protocols [TLSv1.2 TLSv1.3]
    ciphers "HIGH:!aNULL:!MD5"
  }
  
  # Audit logging
  audit {
    enabled true
    log_catalog true
    log_parameter true
    log_statement_once false
    log_level log
    
    # What to audit
    audit_objects [
      users
      sensitive_data
      financial_records
    ]
    
    # Audit retention
    retention_days 365
    archive_enabled true
  }
}
```

---

<div class="examples-footer">
  <h3>More Examples Coming Soon!</h3>
  <p>We're constantly adding new examples. Check back regularly or contribute your own configurations!</p>
  <div class="footer-links">
    <a href="https://github.com/1ay1/vibe/tree/main/examples" class="btn btn-primary">View More Examples</a>
    <a href="https://github.com/1ay1/vibe/blob/main/CONTRIBUTING.md" class="btn btn-secondary">Contribute Examples</a>
  </div>
</div>

<style>
.examples-intro {
  background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
  padding: 2rem;
  border-radius: 1rem;
  margin: 2rem 0;
}

.lead {
  font-size: 1.2rem;
  line-height: 1.6;
  margin: 0;
  color: #2c3e50;
}

.examples-footer {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  padding: 3rem 2rem;
  border-radius: 1rem;
  text-align: center;
  margin: 4rem 0 2rem 0;
}

.examples-footer h3 {
  margin-bottom: 1rem;
  color: white;
}

.examples-footer p {
  margin-bottom: 2rem;
  opacity: 0.9;
}

.footer-links {
  display: flex;
  gap: 1rem;
  justify-content: center;
  flex-wrap: wrap;
}

.btn {
  display: inline-block;
  padding: 0.75rem 1.5rem;
  border-radius: 0.5rem;
  text-decoration: none;
  font-weight: 600;
  transition: all 0.3s ease;
  border: none;
  cursor: pointer;
}

.btn-primary {
  background: #ff6b6b;
  color: white;
}

.btn-primary:hover {
  background: #ff5252;
  transform: translateY(-2px);
}

.btn-secondary {
  background: rgba(255, 255, 255, 0.2);
  color: white;
  backdrop-filter: blur(10px);
}

.btn-secondary:hover {
  background: rgba(255, 255, 255, 0.3);
  transform: translateY(-2px);
}

pre {
  background: #f8f9fa;
  border: 1px solid #e1e8ed;
  border-radius: 0.5rem;
  padding: 1.5rem;
  overflow-x: auto;
  margin: 1.5rem 0;
  line-height: 1.4;
}

code {
  font-family: 'Fira Code', Monaco, Consolas, monospace;
  font-size: 0.9em;
}

h2 {
  color: #2c3e50;
  border-bottom: 2px solid #667eea;
  padding-bottom: 0.5rem;
  margin-top: 4rem;
}

h2:first-of-type {
  margin-top: 2rem;
}

hr {
  border: none;
  height: 2px;
  background: linear-gradient(90deg, #667eea, #764ba2);
  margin: 3rem 0;
  border-radius: 1px;
}

@media (max-width: 768px) {
  .footer-links {
    flex-direction: column;
    align-items: center;
  }
  
  .btn {
    width: 200px;
  }
  
  pre {
    padding: 1rem;
    font-size: 0.8rem;
  }
}
</style>